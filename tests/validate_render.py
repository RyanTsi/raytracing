"""Repeatable GPU checks. Requires numpy and Pillow; no downloads or external scenes."""
import argparse, base64, json, subprocess
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--exe", type=Path, default=ROOT/"build/Release/rtrt.exe")
parser.add_argument("--out", type=Path, default=ROOT/"captures/validation")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
results = {}
def pfm(path):
    with path.open("rb") as f:
        assert f.readline().strip() == b"PF"
        w,h = map(int,f.readline().split())
        endian = "<" if float(f.readline()) < 0 else ">"
        data = np.frombuffer(f.read(), dtype=endian+"f4").reshape(h,w,3).copy()
    assert np.isfinite(data).all() and (data >= 0).all()
    return data
def run(name, frames=32, extra=(), size=(128,80)):
    path = args.out/(name+".ppm")
    command = [str(args.exe.resolve()),"--headless","--width",str(size[0]),"--height",str(size[1]),
        "--frames",str(frames),"--output",str(path),*map(str,extra)]
    p = subprocess.run(command,cwd=ROOT,text=True,encoding="utf-8",errors="replace",
        stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
    (args.out/(name+".log")).write_text(p.stdout,encoding="utf-8")
    if p.returncode: raise RuntimeError(p.stdout)
    stats=json.loads(path.with_suffix(".json").read_text())
    assert stats["gl_errors"] == 0 and stats["spp"] == frames
    results[name]={"command":command,"stats":stats}
    Image.open(path).save(path.with_suffix(".png"))
    print(name, "mean",round(stats["mean_linear_rgb"],6),"GPU median",round(stats["gpu_trace_median_ms"],4),"ms",flush=True)
    return pfm(path.with_suffix(".pfm"))
def mse(a,b): return float(np.mean((a.astype(np.float64)-b)**2))
a=run("bvh",extra=["--force-bvh"])
b=run("brute",extra=["--brute-force"])
assert np.max(np.abs(a-b)) < 1e-4, "BVH and brute force disagree"
results["bvh_max_absolute_difference"]=float(np.max(np.abs(a-b)))
reference=run("reference",4096,["--seed","73"])
s16=run("mis_16",16)
s256=run("mis_256",256)
uniform=run("uniform_16",16,["--uniform-sampling","--no-nee"])
uniform32=run("uniform_32",32,["--uniform-sampling","--no-nee"])
results["linear_mse"]={name:mse(a,reference) for name,a in [("mis_16",s16),("mis_256",s256),("uniform_16",uniform),("uniform_32",uniform32)]}
assert mse(s256,reference)<mse(s16,reference), "Accumulation does not converge"
assert mse(s16,reference)<mse(uniform,reference), "Importance sampling fails to improve this fixture"
nee=run("nee_only",4096,["--no-mis","--seed","31"])
bsdf=run("bsdf_only",4096,["--no-nee","--seed","47"])
means=[float(x.mean()) for x in (reference,nee,bsdf)]
results["estimator_means"]=means
assert max(means)/min(means)<1.06, "NEE / MIS / BSDF energy mismatch"
# A larger existing mesh measures traversal work; tiny view bounds brute-force cost.
backpack=["--scene",ROOT/"assets/backpack/backpack.obj","--bounces","3"]
a=run("backpack_bvh",8,backpack,(96,64))
b=run("backpack_brute",8,[*backpack,"--brute-force"],(96,64))
results["backpack_bvh_mse"]=mse(a,b)
assert mse(a,b)<1e-7, "Large-mesh traversal mismatch"
speed=results["backpack_brute"]["stats"]["gpu_trace_median_ms"]/results["backpack_bvh"]["stats"]["gpu_trace_median_ms"]
results["backpack_traversal_speedup"]=speed
# Full-resolution vs moving preview: same aspect, sample count and integrator.
run("full_resolution",64,size=(960,600))
run("half_resolution",64,["--scale","0.5"],size=(960,600))
# glTF instance transforms / nonuniform normal transforms vs a baked OBJ.
fixture=args.out/"fixtures"
fixture.mkdir(exist_ok=True)
positions=[]; normals=[]
for axis in range(3):
    for sign in [-1,1]:
        u=(axis+1)%3; v=(axis+2)%3
        corners=[]
        for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]:
            p=np.zeros(3); p[axis]=sign; p[u]=x; p[v]=y; corners.append(p)
        for i in ([0,1,2,0,2,3] if sign>0 else [0,2,1,0,3,2]):
            positions.append(corners[i]); n=np.zeros(3); n[axis]=sign; normals.append(n)
pos=np.array(positions,dtype="<f4"); norm=np.array(normals,dtype="<f4")
blob=pos.tobytes()+norm.tobytes()
gltf={"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],
    "nodes":[{"mesh":0,"translation":[3,1,-2],"scale":[2,1,0.5],"rotation":[0,np.sin(0.2),0,np.cos(0.2)]}],
    "buffers":[{"uri":"data:application/octet-stream;base64,"+base64.b64encode(blob).decode(),"byteLength":len(blob)}],
    "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":pos.nbytes},{"buffer":0,"byteOffset":pos.nbytes,"byteLength":norm.nbytes}],
    "accessors":[{"bufferView":0,"componentType":5126,"count":len(pos),"type":"VEC3","min":[-1,-1,-1],"max":[1,1,1]},
        {"bufferView":1,"componentType":5126,"count":len(norm),"type":"VEC3"}],
    "materials":[{"pbrMetallicRoughness":{"baseColorFactor":[0.6,0.2,0.1,1],"metallicFactor":0,"roughnessFactor":0.6}}],
    "meshes":[{"primitives":[{"attributes":{"POSITION":0,"NORMAL":1},"material":0}]}]}
(fixture/"transformed.gltf").write_text(json.dumps(gltf),encoding="utf-8")
rotation=np.array([[np.cos(0.4),0,np.sin(0.4)],[0,1,0],[-np.sin(0.4),0,np.cos(0.4)]])
matrix=rotation@np.diag([2,1,0.5])
p=pos@matrix.T+[3,1,-2]
n=norm@np.linalg.inv(matrix); n/=np.linalg.norm(n,axis=1,keepdims=True)
lines=["mtllib baked.mtl","o baked","usemtl paint"]
lines+=["v "+" ".join(map(str,v)) for v in p]
lines+=["vn "+" ".join(map(str,v)) for v in n]
lines+=["f "+" ".join(f"{j+1}//{j+1}" for j in range(i,i+3)) for i in range(0,len(p),3)]
(fixture/"baked.obj").write_text("\n".join(lines),encoding="utf-8")
(fixture/"baked.mtl").write_text("newmtl paint\nKd 0.6 0.2 0.1\nPr 0.6\nPm 0\n",encoding="utf-8")
a=run("transformed_gltf",32,["--scene",fixture/"transformed.gltf","--bounces","2"])
b=run("baked_obj",32,["--scene",fixture/"baked.obj","--bounces","2"])
results["transform_mse"]=mse(a,b)
assert mse(a,b)<1e-6, "Node/normal transform mismatch"
# Missing normals must be generated, not dereferenced as a null pointer.
(fixture/"no_normals.obj").write_text("o triangle\nv -1 -1 0\nv 1 -1 0\nv 0 1 0\nf 1 2 3\n",encoding="utf-8")
run("missing_normals",4,["--scene",fixture/"no_normals.obj"])
# Determinism across separate processes.
c=run("repeat",32)
assert np.array_equal(c,pfm(args.out/"bvh.pfm")), "Seed is not reproducible"
# Portable resource lookup from another working directory.
p=subprocess.run([str(args.exe.resolve()),"--self-test","--width","96","--height","64"],cwd=args.out,
    stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",timeout=60)
assert p.returncode==0 and "SELF_TEST_PASS" in p.stdout, p.stdout
results["self_test"]=p.stdout
# Reject bad options and absent scenes with a nonzero exit status.
for extra in [["--bounces","99"],["--scene",fixture/"absent.obj"]]:
    p=subprocess.run([str(args.exe.resolve()),"--headless",*map(str,extra)],cwd=args.out,
        stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60)
    assert p.returncode!=0
# Side-by-side exported preview; display transform only, HDR metrics above.
canvas=Image.new("RGB",(128*3,80+24),"#151a20")
draw=ImageDraw.Draw(canvas)
for i,name in enumerate(["uniform_16","mis_16","mis_256"]):
    canvas.paste(Image.open(args.out/(name+".png")),(128*i,24))
    draw.text((128*i+4,5),name,fill="white")
canvas.resize((1152,312)).save(args.out/"comparison.png")
(args.out/"results.json").write_text(json.dumps(results,indent=2),encoding="utf-8")
print("PASS",json.dumps({"mse":results["linear_mse"],"means":means,"backpack_speedup":speed,"transform_mse":results["transform_mse"]}),flush=True)
