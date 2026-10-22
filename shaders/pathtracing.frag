#version 430 core
const float PI = 3.141592653589793;
const float INF = 1e30;
const int STACK_SIZE = 64;
in vec2 screen_uv;
out vec4 FragColor;
uniform sampler2D previousFrame;
uniform sampler2DArray colorTextures, dataTextures;
uniform int debugView;
uniform int frameCount, maxBounces, rngSeed;
uniform bool useNEE, useMIS, useBVH, uniformSampling;
uniform float rayEpsilon, environmentStrength;
struct Camera { vec3 position, front, up; float fov, aspectRatio; };
uniform Camera uCamera;
struct Material { vec4 baseColor; float roughness, metallic, opacity; };
struct TextureMaterial { ivec4 maps0, maps1; vec4 parameters; };
struct Light { vec4 center, normal, a_vec, color; float a_len, b_len, power; };
struct TriangleData { float position[9]; float normal[9]; float uv[6]; uint materialIdx; };
struct BVHNode { vec4 mi, mx; int ls, rs, atom, isleaf; };
layout(std430, binding=1) readonly buffer lightBuffer { Light lights[]; };
layout(std430, binding=2) readonly buffer materialBuffer { Material materials[]; };
layout(std430, binding=3) readonly buffer triangleBuffer { TriangleData triangles_Data[]; };
layout(std430, binding=4) readonly buffer BVH { BVHNode nodes[]; };
layout(std430, binding=6) readonly buffer textureMaterialBuffer { TextureMaterial textureMaterials[]; };
struct Ray { vec3 ori, dir; };
struct Hit { vec3 p, n, gn; float t; uint material; bool light; int primitive; vec2 uv; };
uint rng;
uint alphaSeed;
uint hash(uint v) { v ^= v >> 16; v *= 0x7feb352du; v ^= v >> 15; v *= 0x846ca68bu; return v ^ (v >> 16); }
float random() {
    rng = rng * 747796405u + 2891336453u;
    uint word = ((rng >> ((rng >> 28u) + 4u)) ^ rng) * 277803737u;
    return float((word ^ (word >> 22u)) >> 8) * (1.0 / 16777216.0);
}
vec3 localToWorld(vec3 v, vec3 n) {
    vec3 t = normalize(cross(abs(n.z) < 0.999 ? vec3(0,0,1) : vec3(0,1,0), n));
    return normalize(t*v.x + cross(n,t)*v.y + n*v.z);
}
Hit emptyHit(float limit) { return Hit(vec3(0),vec3(0),vec3(0),limit,0u,false,2147483647,vec2(0)); }
bool boxHit(Ray r, BVHNode b, float limit, out float nearT) {
    nearT = 0.0;
    float farT = limit;
    // Explicit parallel handling avoids 0 * infinity on box planes.
    for (int a=0; a<3; ++a) {
        if (abs(r.dir[a]) < 1e-20) {
            if (r.ori[a] < b.mi[a] || r.ori[a] > b.mx[a]) return false;
        } else {
            float t0 = (b.mi[a]-r.ori[a])/r.dir[a], t1 = (b.mx[a]-r.ori[a])/r.dir[a];
            nearT = max(nearT, min(t0,t1)); farT = min(farT, max(t0,t1));
            if (farT < nearT) return false;
        }
    }
    return farT >= nearT;
}
vec3 positionAt(TriangleData t, int i) { return vec3(t.position[i*3],t.position[i*3+1],t.position[i*3+2]); }
vec3 normalAt(TriangleData t, int i) { return vec3(t.normal[i*3],t.normal[i*3+1],t.normal[i*3+2]); }
vec2 uvAt(TriangleData t,int i) { return vec2(t.uv[i*2],t.uv[i*2+1]); }
vec4 colorMap(int layer,vec2 uv) { return textureLod(colorTextures,vec3(uv,float(layer)),0.0); }
vec4 dataMap(int layer,vec2 uv) { return textureLod(dataTextures,vec3(uv,float(layer)),0.0); }
float opacityAt(uint material,vec2 uv) {
    TextureMaterial tm=textureMaterials[material];
    float alpha=materials[material].opacity;
    if (tm.maps0.x>=0) alpha*=colorMap(tm.maps0.x,uv).a;
    if (tm.maps1.z>=0) alpha*=dataMap(tm.maps1.z,uv).r;
    return clamp(alpha,0.0,1.0);
}
bool triangleHit(Ray r, int index, inout Hit hit) {
    TriangleData t = triangles_Data[index];
    vec3 a=positionAt(t,0), e1=positionAt(t,1)-a, e2=positionAt(t,2)-a;
    vec3 p=cross(r.dir,e2);
    float det=dot(e1,p);
    // Relative determinant threshold also handles tiny imported geometry.
    if (abs(det) <= 1e-8 * length(e1) * length(e2)) return false;
    vec3 s=r.ori-a;
    float u=dot(s,p)/det;
    if (u < 0.0 || u > 1.0) return false;
    vec3 q=cross(s,e1);
    float v=dot(r.dir,q)/det;
    if (v < 0.0 || u+v > 1.0) return false;
    float distance=dot(e2,q)/det;
    if (distance <= rayEpsilon*0.1 || distance > hit.t ||
        (distance == hit.t && index >= hit.primitive)) return false;
    vec2 uv=uvAt(t,0)*(1.0-u-v)+uvAt(t,1)*u+uvAt(t,2)*v;
    TextureMaterial tm=textureMaterials[t.materialIdx];
    if (tm.maps1.w!=0) {
        float alpha=opacityAt(t.materialIdx,uv);
        if (tm.maps1.w==1) { if (alpha<tm.parameters.x) return false; }
        else {
            // Stochastic coverage is independent of traversal order. Do not
            // advance the path RNG for rejected triangles or shadow any-hits.
            float sampleAlpha=float(hash(alphaSeed ^ hash(uint(index))))*(1.0/4294967296.0);
            if (sampleAlpha>=alpha) return false;
        }
    }
    vec3 gn=normalize(cross(e1,e2));
    if (dot(gn,r.dir)>0.0) gn=-gn;
    vec3 n=normalAt(t,0)*(1.0-u-v)+normalAt(t,1)*u+normalAt(t,2)*v;
    n = dot(n,n)>1e-16 ? normalize(n) : gn;
    if (dot(n,gn)<0.0) n=-n;
    if (dot(n,-r.dir)<=0.0) n=gn;
    hit=Hit(r.ori+r.dir*distance,n,gn,distance,t.materialIdx,false,index,uv);
    return true;
}
bool geometryHit(Ray r, inout Hit hit, bool anyHit) {
    bool found=false;
    if (!useBVH) {
        for (int i=0; i<triangles_Data.length(); ++i) {
            if (triangleHit(r,i,hit)) { found=true; if (anyHit) return true; }
        }
        return found;
    }
    if (nodes.length()==0) return false;
    int stack[STACK_SIZE], sp=0;
    stack[sp++]=0;
    while (sp>0) {
        int idx=stack[--sp];
        BVHNode node=nodes[idx];
        float entry;
        if (!boxHit(r,node,hit.t,entry)) continue;
        if (node.isleaf!=0) {
            if (triangleHit(r,node.atom,hit)) { found=true; if (anyHit) return true; }
        } else {
            float tl,tr;
            bool hl=boxHit(r,nodes[node.ls],hit.t,tl);
            bool hr=boxHit(r,nodes[node.rs],hit.t,tr);
            // Far child first, so the near child tightens the closest-hit limit.
            if (hl && hr) {
                stack[sp++]=tl<tr ? node.rs : node.ls;
                stack[sp++]=tl<tr ? node.ls : node.rs;
            } else if (hl) stack[sp++]=node.ls;
            else if (hr) stack[sp++]=node.rs;
        }
    }
    return found;
}
bool lightHit(Ray r, int i, inout Hit hit) {
    Light l=lights[i];
    float den=dot(l.normal.xyz,r.dir);
    if (abs(den)<1e-8) return false;
    float t=dot(l.normal.xyz,l.center.xyz-r.ori)/den;
    if (t<=rayEpsilon*0.1 || t>=hit.t) return false;
    vec3 d=r.ori+r.dir*t-l.center.xyz;
    vec3 b=cross(l.normal.xyz,l.a_vec.xyz);
    if (abs(dot(d,l.a_vec.xyz))>l.a_len*0.5 || abs(dot(d,b))>l.b_len*0.5) return false;
    hit=Hit(r.ori+r.dir*t,l.normal.xyz,l.normal.xyz,t,uint(i),true,-1,vec2(0));
    return true;
}
bool sceneHit(Ray r, inout Hit h, bool anyHit) {
    alphaSeed=hash(rng ^ floatBitsToUint(r.dir.x) ^ hash(floatBitsToUint(r.dir.y)) ^ hash(floatBitsToUint(r.dir.z)));
    bool found=geometryHit(r,h,anyHit);
    if (found && anyHit) return true;
    for (int i=0; i<lights.length(); ++i) {
        if (lightHit(r,i,h)) { found=true; if (anyHit) return true; }
    }
    return found;
}
Material surfaceMaterial(inout Hit hit,out vec3 dielectricF0,out float occlusion) {
    Material m=materials[hit.material];
    TextureMaterial tm=textureMaterials[hit.material];
    if (tm.maps0.x>=0) m.baseColor.rgb*=colorMap(tm.maps0.x,hit.uv).rgb;
    if (tm.maps0.z>=0) m.roughness*=dataMap(tm.maps0.z,hit.uv)[int(tm.parameters.z)];
    if (tm.maps0.w>=0) m.metallic*=dataMap(tm.maps0.w,hit.uv)[int(tm.parameters.w)];
    m.roughness=clamp(m.roughness,0.045,1.0); m.metallic=clamp(m.metallic,0.0,1.0);
    dielectricF0=vec3(0.04);
    if (tm.maps1.x>=0) dielectricF0*=dataMap(tm.maps1.x,hit.uv).rgb;
    occlusion=tm.maps1.y>=0 ? dataMap(tm.maps1.y,hit.uv).r : 1.0;
    if (tm.maps0.y>=0) {
        TriangleData t=triangles_Data[hit.primitive];
        vec3 e1=positionAt(t,1)-positionAt(t,0), e2=positionAt(t,2)-positionAt(t,0);
        vec2 d1=uvAt(t,1)-uvAt(t,0), d2=uvAt(t,2)-uvAt(t,0);
        float determinant=d1.x*d2.y-d1.y*d2.x;
        if (abs(determinant)>1e-12) {
            vec3 tangent=(e1*d2.y-e2*d1.y)/determinant;
            tangent-=hit.n*dot(tangent,hit.n);
            if (dot(tangent,tangent)>1e-16) {
                tangent=normalize(tangent);
                vec3 bitangent=(e2*d1.x-e1*d2.x)/determinant;
                float handedness=dot(cross(tangent,bitangent),hit.n)<0.0 ? -1.0 : 1.0;
                bitangent=normalize(cross(hit.n,tangent))*handedness;
                vec3 normal=dataMap(tm.maps0.y,hit.uv).rgb*2.0-1.0;
                normal.xy*=tm.parameters.y;
                if (dot(normal,normal)>1e-12) {
                    normal=normalize(tangent*normal.x+bitangent*normal.y+hit.n*normal.z);
                    if (dot(normal,hit.gn)>0.0) hit.n=normal;
                }
            }
        }
    }
    return m;
}
vec3 fresnel(float cosTheta, vec3 f0) { return f0+(1.0-f0)*pow(clamp(1.0-cosTheta,0.0,1.0),5.0); }
float ggxD(float nh, float alpha) {
    float a2=alpha*alpha, d=nh*nh*(a2-1.0)+1.0;
    return a2/(PI*d*d);
}
float smithG1(float nv, float alpha) {
    return 2.0*nv/(nv+sqrt(alpha*alpha+(1.0-alpha*alpha)*nv*nv));
}
float specProbability(Material m) { return mix(0.25,0.85,m.metallic); }
vec3 bsdf(vec3 n, vec3 v, vec3 l, Material m, vec3 dielectricF0, out float pdf) {
    float nv=dot(n,v), nl=dot(n,l);
    pdf=0.0;
    if (nv<=0.0 || nl<=0.0 || dot(v+l,v+l)<1e-16) return vec3(0);
    vec3 h=normalize(v+l);
    float nh=max(dot(n,h),0.0), vh=max(dot(v,h),1e-8);
    float alpha=m.roughness*m.roughness;
    float d=ggxD(nh,alpha);
    vec3 f=fresnel(vh,mix(dielectricF0,m.baseColor.rgb,m.metallic));
    vec3 diffuse=(1.0-f)*(1.0-m.metallic)*m.baseColor.rgb/PI;
    vec3 spec=d*smithG1(nv,alpha)*smithG1(nl,alpha)*f/(4.0*nv*nl);
    float p=specProbability(m);
    pdf=uniformSampling ? 1.0/(2.0*PI) : mix(nl/PI,d*nh/(4.0*vh),p);
    return diffuse+spec;
}
vec3 sampleBSDF(vec3 n, vec3 v, Material m) {
    float selector=random(), u=random(), phi=2.0*PI*random();
    if (uniformSampling) {
        float z=u, r=sqrt(max(0.0,1.0-z*z));
        return localToWorld(vec3(r*cos(phi),r*sin(phi),z),n);
    }
    if (selector<specProbability(m)) {
        float alpha=m.roughness*m.roughness;
        float z=sqrt((1.0-u)/(1.0+(alpha*alpha-1.0)*u));
        float r=sqrt(max(0.0,1.0-z*z));
        vec3 h=localToWorld(vec3(r*cos(phi),r*sin(phi),z),n);
        return reflect(-v,h);
    }
    float r=sqrt(u);
    return localToWorld(vec3(r*cos(phi),r*sin(phi),sqrt(1.0-u)),n);
}
float powerWeight(float a, float b) {
    float scale=max(a,b);
    a/=scale; b/=scale;
    return (a*a)/(a*a+b*b);
}
float lightPDF(int i, vec3 from, vec3 to) {
    Light l=lights[i];
    vec3 d=to-from;
    float distance2=dot(d,d);
    float cosine=dot(l.normal.xyz,-normalize(d));
    if (cosine<=0.0) return 0.0;
    return distance2/(cosine*l.a_len*l.b_len*float(lights.length()));
}
vec3 trace(Ray r) {
    vec3 radiance=vec3(0), throughput=vec3(1), previousPoint=r.ori;
    float previousPDF=0.0;
    for (int bounce=0; bounce<=maxBounces; ++bounce) {
        Hit hit=emptyHit(INF);
        if (!sceneHit(r,hit,false)) {
            radiance+=throughput*vec3(environmentStrength);
            break;
        }
        if (hit.light) {
            Light l=lights[hit.material];
            if (dot(l.normal.xyz,-r.dir)>0.0) {
                float weight=1.0;
                if (bounce>0 && useNEE && useMIS)
                    weight=powerWeight(previousPDF,lightPDF(int(hit.material),previousPoint,hit.p));
                else if (bounce>0 && useNEE) weight=0.0;
                // Emitted radiance has no extra cosine factor.
                radiance+=throughput*l.color.rgb*l.power*weight;
            }
            break;
        }
        if (bounce==maxBounces) break;
        vec3 dielectricF0; float occlusion;
        Material m=surfaceMaterial(hit,dielectricF0,occlusion);
        vec3 v=-r.dir;
        if (useNEE && lights.length()>0) {
            int index=min(int(random()*float(lights.length())),lights.length()-1);
            Light l=lights[index];
            vec3 b=cross(l.normal.xyz,l.a_vec.xyz);
            vec3 point=l.center.xyz+(random()-0.5)*l.a_len*l.a_vec.xyz+(random()-0.5)*l.b_len*b;
            vec3 direction=normalize(point-hit.p);
            float nl=dot(hit.n,direction), lp=lightPDF(index,hit.p,point);
            if (nl>0.0 && dot(hit.gn,direction)>0.0 && lp>0.0) {
                Ray shadow=Ray(hit.p+hit.gn*rayEpsilon,direction);
                // Aim from the offset origin to the same sampled light point.
                vec3 sd=point-shadow.ori;
                shadow.dir=normalize(sd);
                Hit blocker=emptyHit(length(sd)-rayEpsilon);
                if (!sceneHit(shadow,blocker,true)) {
                    float bp;
                    vec3 f=bsdf(hit.n,v,direction,m,dielectricF0,bp);
                    float weight=useMIS ? powerWeight(lp,bp) : 1.0;
                    radiance+=throughput*f*nl*l.color.rgb*l.power*(weight/lp);
                }
            }
        }
        vec3 direction=sampleBSDF(hit.n,v,m);
        if (dot(hit.gn,direction)<=0.0) break;
        float pdf;
        vec3 f=bsdf(hit.n,v,direction,m,dielectricF0,pdf);
        if (pdf<=1e-20) break;
        throughput*=f*max(dot(hit.n,direction),0.0)*occlusion/pdf;
        previousPDF=pdf; previousPoint=hit.p;
        // Unbiased stochastic termination; hard depth still limits path length.
        if (bounce>=3) {
            float survival=clamp(max(throughput.r,max(throughput.g,throughput.b)),0.05,0.95);
            if (random()>survival) break;
            throughput/=survival;
        }
        r=Ray(hit.p+hit.gn*rayEpsilon,normalize(direction));
    }
    return radiance;
}
void main() {
    ivec2 pixel=ivec2(gl_FragCoord.xy), size=textureSize(previousFrame,0);
    uint id=uint(pixel.x)+uint(pixel.y)*uint(size.x);
    rng=hash(id ^ hash(uint(frameCount)+uint(rngSeed)*0x9e3779b9u));
    vec2 jitter=vec2(random(),random());
    vec2 ndc=(vec2(pixel)+jitter)/vec2(size)*2.0-1.0;
    vec3 right=normalize(cross(uCamera.front,uCamera.up));
    vec3 up=normalize(cross(right,uCamera.front));
    float scale=tan(radians(uCamera.fov)*0.5);
    Ray ray=Ray(uCamera.position,normalize(uCamera.front+scale*(right*ndc.x*uCamera.aspectRatio+up*ndc.y)));
    vec3 value;
    if (debugView>0) {
        Hit hit=emptyHit(INF);
        if (!sceneHit(ray,hit,false)) value=vec3(0);
        else if (hit.light) value=lights[hit.material].color.rgb;
        else {
            vec3 f0; float ao;
            Material m=surfaceMaterial(hit,f0,ao);
            value=debugView==1 ? m.baseColor.rgb : hit.n*0.5+0.5;
        }
    } else value=trace(ray);
    // Read only initialized history, preserve linear HDR throughout accumulation.
    if (frameCount>0) {
        vec3 previous=texelFetch(previousFrame,pixel,0).rgb;
        value=previous+(value-previous)/float(frameCount+1);
    }
    FragColor=vec4(value,1);
}
