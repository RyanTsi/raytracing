# Third-party notices

The root [MIT license](LICENSE) covers project-authored renderer code, shaders,
tests, scripts and documentation, except where a file carries a separate notice.
It does not replace the licenses of dependencies, third-party models, textures
or externally supplied content. No ownership or endorsement by those authors is
claimed.

The libraries in `lib/` are vendored source trees in this checkout, rather than
Git submodule links. Keep their copyright and license files with redistributed
source. A convenient collection of the relevant local license texts is in
[LICENSES/THIRD_PARTY-LICENSE-TEXTS.txt](LICENSES/THIRD_PARTY-LICENSE-TEXTS.txt).
For a binary distribution, include this notice, the root license, that collection,
and [LICENSES/Apache-2.0.txt](LICENSES/Apache-2.0.txt).

## Libraries

| Component | Location and notice | Applicable license |
|---|---|---|
| GLFW | `lib/glfw/LICENSE.md`; Marcus Geelnard and Camilla Löwy | zlib/libpng style license |
| GLM | `lib/glm/copying.txt`; G-Truc Creation | MIT option selected from its dual-license offer |
| Dear ImGui, including GLFW/OpenGL backends | `lib/imgui/LICENSE.txt`; Omar Cornut and contributors | MIT |
| Assimp | `lib/assimp/LICENSE` and per-file headers; Assimp contributors | BSD-3-Clause, with upstream exceptions for test models |
| stb_image | Final license block in `lib/stb_image/include/stb_image.h`; Sean Barrett and contributors | MIT option selected from MIT / public domain |
| GLAD generated loader, version 2.0.8 | SPDX headers in `lib/glad/src/gl.c` and `lib/glad/include/glad/gl.h` | `(WTFPL OR CC0-1.0) AND Apache-2.0`; CC0-1.0 option selected, with Apache-2.0 retained |
| Khronos platform header | Header notice in `lib/glad/include/KHR/khrplatform.h`; The Khronos Group Inc. | MIT-style permission notice |

GLAD's generated-file SPDX notice is the relevant notice for this bundled loader;
the generator's own MIT license does not erase the generated Khronos specification
notices. See [GLAD upstream licensing](https://github.com/Dav1dde/glad/blob/v2.0.8/LICENSE),
[CC0-1.0](https://creativecommons.org/publicdomain/zero/1.0/), and the bundled
Apache-2.0 license text.

Assimp also vendors libraries with their own notices, including zlib
(`contrib/zlib/LICENSE`), Poly2Tri (`contrib/poly2tri/LICENSE`),
OpenDDL Parser (`contrib/openddlparser/LICENSE`), RapidJSON
(`contrib/rapidjson/license.txt`), earcut.hpp (`contrib/earcut-hpp/LICENSE`),
utf8cpp (`contrib/utf8cpp/source/utf8.h`), clipper
(`contrib/clipper/clipper.hpp`), Open3DGC (per-file headers), minizip/unzip
(per-file headers), pugixml (per-file notices), and zip
(`contrib/zip/UNLICENSE`). Optional dependencies and upstream examples/tests
retain their own licenses as well. Shipping an upstream test-model directory is
not covered merely by the Assimp BSD license.

The historical code tree uses patterns taught by LearnOpenGL. Educational
reference: [Joey de Vries, LearnOpenGL](https://learnopengl.com/),
[@JoeyDeVriez](https://twitter.com/JoeyDeVriez).
This acknowledgment is not a relicensing of that website: its
[About page](https://learnopengl.com/About) separately specifies CC BY-NC 4.0
for code examples and CC BY 4.0 for images/videos. Third-party example material
copied from it, if any, retains its original terms; the root MIT license covers
only code the project author is entitled to license.

## Bundled backpack model and textures

**Survival Guitar Backpack**, by **Berk Gedik**, is distributed under
[Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/).

- Original model: [Sketchfab](https://sketchfab.com/3d-models/survival-guitar-backpack-799f8c4511f84fab8c3f12887f7e6b36).
- Artist portfolio: [Berk Gedik](https://berkgedik.artstation.com/projects/nQnDBe).
- Local files: `assets/backpack/`, including the mesh and accompanying textures.
- The local package credits **Joey de Vries** with changing material assignment
  and renaming albedo to `diffuse` and metallic to `specular` for the LearnOpenGL
  model-loading chapter. Keep [the original attribution](assets/backpack/source_attribution.txt).
- This project's rendering, interpretation of those texture channels and any
  resulting lighting are additional changes; they are not an artist-endorsed
  reconstruction of the original appearance.

The Sketchfab source listed CC Attribution when checked on 2026-10-22.
CC BY attribution includes the creator, source, license and indication of changes.
The backpack is **not MIT-licensed**. For a video description, a suitable credit is:

> “Survival Guitar Backpack” by Berk Gedik, CC BY 4.0.
> Source: https://sketchfab.com/3d-models/survival-guitar-backpack-799f8c4511f84fab8c3f12887f7e6b36
> License: https://creativecommons.org/licenses/by/4.0/
> Material assignment/texture naming adapted by Joey de Vries; rendered and further
> adjusted in RyanTsi/raytracing.

## Other assets and external models

Project-authored simple cube/plane test scenes are supplied as MIT-licensed
examples under the root license. Third-party content is governed by its own
notices, irrespective of its file extension or location.

The previously bundled, unreferenced `assets/image.png` had no recorded source
or redistribution license. It is excluded from the rewritten public history and
release tree; a local quarantined copy and the original-history backup are not
release assets.

The externally supplied **诺姆** PMX character and weapon are **not included** in
this repository or licensed under MIT. Their creator's rules explicitly prohibit
redistribution and commercial use. Model creation credit: **去看流星雨**
(BiliBili: **-去看流星雨-**); the supplied notice names **miHoYo** as the final
interpreter of its rules. See [docs/LICENSING.md](docs/LICENSING.md) before using
character renders in a public video.

Generated render outputs inherit any obligations associated with the depicted
assets; rendering a model does not convert its terms into the renderer's MIT
license.
