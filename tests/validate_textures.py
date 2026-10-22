"""GPU regression tests for texture orientation, color space, alpha and normals.

Requires numpy and Pillow. Generates its own OBJ/MTL/PNG fixtures under the
ignored captures directory. --character accepts an external model file; no
external character or textures are copied into the repository.
"""
from __future__ import annotations

import argparse
import base64
import io
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
LINEAR_128 = ((128.0 / 255.0 + 0.055) / 1.055) ** 2.4


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def read_pfm(path: Path) -> np.ndarray:
    with path.open("rb") as stream:
        require(stream.readline().strip() == b"PF", f"Not an RGB PFM: {path}")
        width, height = map(int, stream.readline().split())
        scale = float(stream.readline())
        require(scale != 0, "PFM scale cannot be zero")
        pixels = np.frombuffer(stream.read(), dtype="<f4" if scale < 0 else ">f4")
    require(pixels.size == width * height * 3, f"Truncated PFM: {path}")
    result = pixels.reshape(height, width, 3).astype(np.float64) * abs(scale)
    require(bool(np.isfinite(result).all()), f"Non-finite HDR values: {path}")
    require(bool((result >= 0).all()), f"Negative HDR values: {path}")
    # The application and PFM both store row zero at the image bottom.
    return result


def normalize(vector: np.ndarray) -> np.ndarray:
    length = float(np.linalg.norm(vector))
    require(length > 1e-12, "Degenerate camera basis in metadata")
    return vector / length


def project_world(point: tuple[float, float, float], stats: dict) -> tuple[int, int]:
    camera = stats["camera"]
    front = normalize(np.array(camera["front"], dtype=float))
    right = normalize(np.cross(front, np.array(camera["up"], dtype=float)))
    up = normalize(np.cross(right, front))
    delta = np.array(point, dtype=float) - np.array(camera["position"], dtype=float)
    depth = float(np.dot(delta, front))
    require(depth > 0, f"Fixture point behind the camera: {point}")
    half_height = np.tan(np.deg2rad(float(camera["fov_degrees"])) * 0.5)
    aspect = float(camera["aspect"])
    ndc = np.array([
        np.dot(delta, right) / (depth * half_height * aspect),
        np.dot(delta, up) / (depth * half_height),
    ])
    require(bool((np.abs(ndc) < 0.9).all()), f"Fixture point outside safe image area: {point}, {ndc}")
    pixel = (ndc * 0.5 + 0.5) * [stats["width"], stats["height"]] - 0.5
    x, y = map(lambda value: int(round(float(value))), pixel)
    return x, y


def sample_world(image: np.ndarray, stats: dict, point: tuple[float, float, float]) -> np.ndarray:
    x, y = project_world(point, stats)
    height, width = image.shape[:2]
    require(1 <= x < width - 1 and 1 <= y < height - 1, "Fixture sample patch reaches image border")
    return image[y - 1:y + 2, x - 1:x + 2].mean(axis=(0, 1))


def write_quad_fixture(
    directory: Path,
    name: str,
    texture: str | None = None,
    normal: str | None = None,
    background: bool = False,
) -> Path:
    """A CCW XY quad faces +Z, with standard OBJ V=0 bottom / V=1 top."""
    material = ["newmtl surface", "Kd 1 1 1", "Pr 1", "Pm 0", "d 1"]
    if texture:
        material.append(f"map_Kd {texture}")
    if normal:
        # Assimp exposes OBJ map_Bump as HEIGHT. The renderer explicitly
        # recognizes an RGB normal map by the normal_* file name.
        material.append(f"map_Bump {normal}")
    if background:
        material += ["", "newmtl background", "Kd 0.05 0.2 0.4", "Pr 1", "Pm 0", "d 1"]
    (directory / f"{name}.mtl").write_text("\n".join(material) + "\n", encoding="utf-8")
    lines = [
        f"mtllib {name}.mtl", "o quad", "usemtl surface",
        "v -1 -1 0", "v 1 -1 0", "v 1 1 0", "v -1 1 0",
        "vt 0 0", "vt 1 0", "vt 1 1", "vt 0 1", "vn 0 0 1",
        "f 1/1/1 2/2/1 3/3/1", "f 1/1/1 3/3/1 4/4/1",
    ]
    if background:
        lines += [
            "o backing_quad", "usemtl background",
            "v -1 -1 -0.15", "v 1 -1 -0.15", "v 1 1 -0.15", "v -1 1 -0.15",
            "f 5/1/1 6/2/1 7/3/1", "f 5/1/1 7/3/1 8/4/1",
        ]
    path = directory / f"{name}.obj"
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return path


def write_embedded_gltf(
    directory: Path, name: str, color: tuple[int, int, int], x0: float, x1: float,
) -> Path:
    """Each independent model deliberately owns its own embedded texture *0."""
    positions = np.array([
        [x0, -0.75, 0], [x1, -0.75, 0], [x1, 0.75, 0], [x0, 0.75, 0],
    ], dtype="<f4")
    normals = np.tile(np.array([0, 0, 1], dtype="<f4"), (4, 1))
    uv = np.array([[0, 0], [1, 0], [1, 1], [0, 1]], dtype="<f4")
    indices = np.array([0, 1, 2, 0, 2, 3], dtype="<u2")
    arrays = [positions, normals, uv, indices]
    blob = b"".join(array.tobytes() for array in arrays)
    views = []
    offset = 0
    for index, array in enumerate(arrays):
        views.append({
            "buffer": 0, "byteOffset": offset, "byteLength": array.nbytes,
            "target": 34963 if index == 3 else 34962,
        })
        offset += array.nbytes
    image = io.BytesIO()
    Image.new("RGB", (64, 64), color).save(image, format="PNG")
    document = {
        "asset": {"version": "2.0"}, "scene": 0,
        "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0}],
        "buffers": [{
            "uri": "data:application/octet-stream;base64," + base64.b64encode(blob).decode("ascii"),
            "byteLength": len(blob),
        }],
        "bufferViews": views,
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3",
             "min": positions.min(axis=0).tolist(), "max": positions.max(axis=0).tolist()},
            {"bufferView": 1, "componentType": 5126, "count": 4, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": 4, "type": "VEC2"},
            {"bufferView": 3, "componentType": 5123, "count": 6, "type": "SCALAR"},
        ],
        "images": [{
            "uri": "data:image/png;base64," + base64.b64encode(image.getvalue()).decode("ascii"),
        }],
        "textures": [{"source": 0}],
        "materials": [{"pbrMetallicRoughness": {
            "baseColorFactor": [1, 1, 1, 1], "baseColorTexture": {"index": 0},
            "metallicFactor": 0, "roughnessFactor": 1,
        }}],
        "meshes": [{"primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
            "indices": 3, "material": 0,
        }]}],
    }
    path = directory / f"{name}.gltf"
    path.write_text(json.dumps(document), encoding="utf-8")
    return path


class Validation:
    def __init__(self, executable: Path, output: Path) -> None:
        self.executable = executable.resolve()
        self.output = output.resolve()
        self.output.mkdir(parents=True, exist_ok=True)
        self.report: dict = {
            "description": "Real GPU texture regression; albedo/normal use linear HDR PFM values.",
            "linear_srgb_128": LINEAR_128,
            "runs": {},
            "checks": {},
        }

    def save_report(self) -> None:
        (self.output / "results.json").write_text(
            json.dumps(self.report, indent=2, ensure_ascii=False), encoding="utf-8"
        )

    def command(self, name: str, model: Path, view: str, frames: int, size: tuple[int, int]) -> list[str]:
        return [
            str(self.executable), "--headless", "--scene", str(model.resolve()),
            "--view", view, "--width", str(size[0]), "--height", str(size[1]),
            "--frames", str(frames), "--bounces", "3", "--seed", "19",
            "--output", str(self.output / f"{name}.ppm"),
        ]

    def run(
        self, name: str, model: Path, view: str = "albedo",
        frames: int = 4, size: tuple[int, int] = (64, 64),
        extra: tuple[str | Path, ...] = (),
    ) -> tuple[np.ndarray, dict]:
        command = self.command(name, model, view, frames, size) + [str(value) for value in extra]
        process = subprocess.run(
            command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, encoding="utf-8", errors="replace", timeout=240,
        )
        (self.output / f"{name}.log").write_text(process.stdout, encoding="utf-8")
        require(process.returncode == 0, f"{name} exited {process.returncode}:\n{process.stdout}")
        stats = json.loads((self.output / f"{name}.json").read_text(encoding="utf-8"))
        require(stats["gl_errors"] == 0, f"{name}: OpenGL errors")
        require(stats["spp"] == frames, f"{name}: wrong accumulated sample count")
        expected_view = {"path": 0, "albedo": 1, "normal": 2}[view]
        require(stats["debug_view"] == expected_view, f"{name}: debug view not selected")
        image = read_pfm(self.output / f"{name}.pfm")
        require(image.shape[:2] == (size[1], size[0]), f"{name}: unexpected output dimensions")
        Image.open(self.output / f"{name}.ppm").save(self.output / f"{name}.png")
        self.report["runs"][name] = {"command": command, "stats": stats}
        self.save_report()
        print(f"{name}: {size[0]}x{size[1]}, {frames} spp, GL errors=0", flush=True)
        return image, stats

    def check_close(self, name: str, actual: np.ndarray, expected: np.ndarray, tolerance: float = 0.008) -> None:
        actual = np.asarray(actual, dtype=float)
        expected = np.asarray(expected, dtype=float)
        error = float(np.max(np.abs(actual - expected)))
        self.report["checks"][name] = {
            "actual": actual.tolist(), "expected": expected.tolist(),
            "maximum_absolute_error": error, "tolerance": tolerance,
        }
        self.save_report()
        require(error <= tolerance, f"{name}: expected {expected}, got {actual}, max error {error}")

    def expect_missing_texture(self, model: Path) -> None:
        command = self.command("missing_texture", model, "albedo", 1, (64, 64))
        process = subprocess.run(
            command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, encoding="utf-8", errors="replace", timeout=60,
        )
        (self.output / "missing_texture.log").write_text(process.stdout, encoding="utf-8")
        # The selected implementation fails closed for an actually referenced
        # absent image, instead of silently displaying a solid-color fallback.
        require(process.returncode != 0, "Missing referenced texture was silently ignored")
        require("missing texture:" in process.stdout.lower(), f"Missing-texture error is unclear:\n{process.stdout}")
        require("intentionally_missing_texture.png" in process.stdout, "Missing-texture error omits the file")
        self.report["checks"]["missing_texture"] = {
            "behavior": "explicit nonzero exit for missing referenced image",
            "returncode": process.returncode, "command": command,
        }
        self.save_report()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=ROOT / "build/Release/rtrt.exe")
    parser.add_argument("--out", type=Path, default=ROOT / "captures/texture-validation")
    parser.add_argument("--character", type=Path, help="Optional external character model file, e.g. character.pmx")
    args = parser.parse_args()
    require(args.exe.is_file(), f"Executable not found: {args.exe}")
    if args.character:
        require(args.character.is_file(), "--character must point to a model file, not a directory")
    validation = Validation(args.exe, args.out)
    fixtures = validation.output / "fixtures"
    fixtures.mkdir(exist_ok=True)
    try:
        quadrant = np.zeros((64, 64, 4), dtype=np.uint8)
        quadrant[:32, :32] = [128, 0, 0, 255]
        quadrant[:32, 32:] = [0, 128, 0, 255]
        quadrant[32:, :32] = [0, 0, 128, 255]
        quadrant[32:, 32:] = [255, 255, 255, 255]
        Image.fromarray(quadrant).save(fixtures / "quadrants.png")
        alpha = quadrant.copy()
        alpha[:32, :32, 3] = 0
        Image.fromarray(alpha).save(fixtures / "alpha_quadrants.png")
        Image.new("RGB", (64, 64), (128, 128, 255)).save(fixtures / "normal_flat.png")
        Image.new("RGB", (64, 64), (218, 128, 218)).save(fixtures / "normal_tilt.png")

        opaque = write_quad_fixture(fixtures, "quadrants", texture="quadrants.png")
        alpha_model = write_quad_fixture(fixtures, "alpha", texture="alpha_quadrants.png", background=True)
        flat = write_quad_fixture(fixtures, "normal_flat", normal="normal_flat.png")
        tilted = write_quad_fixture(fixtures, "normal_tilt", normal="normal_tilt.png")
        missing = write_quad_fixture(fixtures, "missing", texture="intentionally_missing_texture.png")

        points = {
            "top_left": (-0.5, 0.5, 0.0), "top_right": (0.5, 0.5, 0.0),
            "bottom_left": (-0.5, -0.5, 0.0), "bottom_right": (0.5, -0.5, 0.0),
        }
        expected = {
            "top_left": np.array([LINEAR_128, 0, 0]),
            "top_right": np.array([0, LINEAR_128, 0]),
            "bottom_left": np.array([0, 0, LINEAR_128]),
            "bottom_right": np.array([1, 1, 1]),
        }
        image, stats = validation.run("uv_srgb", opaque)
        for name, point in points.items():
            validation.check_close(f"uv_srgb_{name}", sample_world(image, stats, point), expected[name])
        validation.check_close("debug_miss_is_black", image[0, 0], np.zeros(3), tolerance=1e-7)

        # Keep both files in the same folder: path-based caching of the Assimp
        # embedded name "*0" must not alias images belonging to separate models.
        # The second load also expands the material vector after the first mesh
        # exists, exposing any remaining pointers into the old material storage.
        embedded_red = write_embedded_gltf(fixtures, "embedded_red", (128, 0, 0), -1.75, -0.25)
        embedded_green = write_embedded_gltf(fixtures, "embedded_green", (0, 128, 0), 0.25, 1.75)
        image, stats = validation.run(
            "embedded_attachment", embedded_red, extra=("--attachment", embedded_green),
        )
        red_sample = sample_world(image, stats, (-1.0, 0.0, 0.0))
        green_sample = sample_world(image, stats, (1.0, 0.0, 0.0))
        validation.check_close("embedded_scene_red", red_sample, np.array([LINEAR_128, 0, 0]))
        validation.check_close("embedded_attachment_green", green_sample, np.array([0, LINEAR_128, 0]))
        require(float(np.linalg.norm(red_sample - green_sample)) > 0.25, "Embedded images alias across models")
        require(stats["attachment_count"] == 1, "Attachment was not recorded")
        require(stats["meshes"] >= 2 and stats["materials"] >= 2, "Merged model geometry/materials are missing")
        require(stats["color_textures"] == 2, "Separate embedded images were merged or not loaded")
        validation.report["checks"]["embedded_attachment_isolation"] = {
            "attachment_count": stats["attachment_count"], "meshes": stats["meshes"],
            "materials": stats["materials"], "color_textures": stats["color_textures"],
            "scope": "Material indices survive expansion; separate model-local embedded texture 0 images remain distinct.",
        }
        validation.save_report()

        image, stats = validation.run("alpha_coverage", alpha_model)
        validation.check_close(
            "alpha_zero_reveals_backing_surface",
            sample_world(image, stats, points["top_left"]), np.array([0.05, 0.2, 0.4]),
        )
        for name in ("top_right", "bottom_left", "bottom_right"):
            validation.check_close(
                f"alpha_opaque_{name}", sample_world(image, stats, points[name]), expected[name]
            )

        flat_image, flat_stats = validation.run("normal_flat", flat, view="normal")
        tilt_image, tilt_stats = validation.run("normal_tilt", tilted, view="normal")
        flat_expected = normalize(np.array([128, 128, 255]) / 255.0 * 2.0 - 1.0) * 0.5 + 0.5
        tilt_expected = normalize(np.array([218, 128, 218]) / 255.0 * 2.0 - 1.0) * 0.5 + 0.5
        flat_value = sample_world(flat_image, flat_stats, (0, 0, 0))
        tilt_value = sample_world(tilt_image, tilt_stats, (0, 0, 0))
        validation.check_close("flat_normal_uses_linear_data", flat_value, flat_expected)
        validation.check_close("tilted_tangent_normal", tilt_value, tilt_expected)
        require(float(np.linalg.norm(flat_value - tilt_value)) > 0.25, "Normal texture does not affect shading normal")
        validation.expect_missing_texture(missing)
        validation.run("textured_path", opaque, view="path", frames=32)

        # Use the existing attributed backpack, without depending on renderer-
        # specific texture-count or material-count metadata.
        backpack = ROOT / "assets/backpack/backpack.obj"
        backpack_image, _ = validation.run("backpack_albedo", backpack, frames=4)
        color_range = np.max(backpack_image, axis=2) - np.min(backpack_image, axis=2)
        colored = (color_range > 0.02) & (backpack_image.max(axis=2) > 0.025)
        require(int(colored.sum()) > 20, "Backpack albedo remains grayscale or blank")
        unique_colors = len(np.unique(np.round(backpack_image[colored] * 255).astype(int), axis=0))
        require(unique_colors > 20, "Backpack albedo has insufficient spatial color variation")
        validation.report["checks"]["backpack_texture_variation"] = {
            "colored_pixels": int(colored.sum()), "distinct_quantized_colors": unique_colors,
        }
        validation.run("backpack_path", backpack, view="path", frames=32)

        if args.character:
            character, _ = validation.run(
                "character_albedo", args.character, frames=8, size=(96, 128)
            )
            visible = character.max(axis=2) > 0.025
            require(int(visible.sum()) > 64, "Character render is blank or nearly empty")
            require(float(character[visible].std()) > 0.01, "Character render lacks visible variation")
            validation.report["checks"]["character_nonempty"] = {
                "model": str(args.character.resolve()), "visible_pixels": int(visible.sum()),
                "visible_rgb_std": float(character[visible].std()),
                "scope": "Nonempty finite albedo and path render; not a proof of animation or MMD-specific toon matching.",
            }
            validation.run(
                "character_path", args.character, view="path", frames=32, size=(192, 256)
            )
        validation.report["status"] = "passed"
        validation.save_report()
        print("PASS: UV orientation, sRGB decode, embedded attachment isolation, alpha coverage, data-space normals, missing image, real textured models", flush=True)
        return 0
    except Exception as error:
        validation.report["status"] = "failed"
        validation.report["error"] = f"{type(error).__name__}: {error}"
        validation.save_report()
        raise


if __name__ == "__main__":
    sys.exit(main())
