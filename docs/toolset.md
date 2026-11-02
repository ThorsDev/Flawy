# Flawy – Toolset

A Flawy egy egyszerű, multiplatform, C++20-ban írt game engine. Minden függőség nyílt forrású, és megengedő licencű (MIT / zlib / BSD / public domain).

A függőségeket nem package managerből húzzuk. A `scripts/install.bat` / `scripts/install.ps1` (Windows) vagy a `scripts/install.sh` (Linux/macOS) klónozza le őket gitről a `vendor/` mappába. A verziók rögzítettek. A forrásból fordított függőségek a `scripts/dependencies.txt`-ben vannak. Az előre fordított binárisok (Slang, DXC) a `scripts/prebuilt.txt`-ben vannak platformonként; ezeket az install script a GitHub release-ekből tölti le, és kicsomagolja.

## Alap stack

| Terület | Könyvtár | Verzió | Licenc | Repo |
|---|---|---|---|---|
| Ablak, input, platform | SDL3 | release-3.4.18 | zlib | https://github.com/libsdl-org/SDL |
| Rendering | bgfx (bgfx.cmake, bx és bimg is) | v1.166.9546-589 | BSD-2 | https://github.com/bkaradzic/bgfx.cmake |
| Hang | miniaudio | 0.11.25 | MIT-0 / Public Domain | https://github.com/mackron/miniaudio |
| Modellek (glTF 2.0) | fastgltf | v0.9.1 | MIT | https://github.com/spnda/fastgltf |
| JSON parser (a fastgltf használja) | simdjson | v5.0.4 | Apache-2.0 / MIT | https://github.com/simdjson/simdjson |
| ECS | EnTT | v3.16.0 | MIT | https://github.com/skypjack/entt |
| Scripting | AngelScript (+ add-onok) | v2.38.0 | zlib | https://github.com/anjo76/angelscript |
| Fizika | Jolt Physics | v5.6.0 | MIT | https://github.com/jrouwe/JoltPhysics |
| Csontváz-animáció | ozz-animation | 0.17.0 | MIT | https://github.com/guillaumeblanc/ozz-animation |
| Modellek (FBX) | ufbx | v0.23.1 | MIT / Unlicense | https://github.com/ufbx/ufbx |
| Modellek (OBJ) | fast_obj | v1.3 | MIT | https://github.com/thisistherk/fast_obj |
| Matek | GLM | 1.0.3 | MIT | https://github.com/g-truc/glm |
| Debug UI | Dear ImGui | v1.92.9b | MIT | https://github.com/ocornut/imgui |
| Shader compiler | Slang (prebuilt release) | v2026.19 | Apache-2.0 WITH LLVM-exception | https://github.com/shader-slang/slang |
| DXIL compiler (D3D12) | DXC (prebuilt release) | v1.9.2609 | NCSA / MIT | https://github.com/microsoft/DirectXShaderCompiler |
| Profiling | Tracy | v0.14.1 | BSD-3 | https://github.com/wolfpld/tracy |
| Logolás | spdlog | v1.17.0 | MIT | https://github.com/gabime/spdlog |

## Miért ezek?

- **SDL3:** a platformréteg, vagyis ablak, input, eventek, fájlrendszer és időzítés. Statikusan linkeljük, így nincs DLL-függőség.
- **bgfx:** egyetlen API-val fut D3D11/12, Vulkan, Metal és OpenGL/GLES alatt, így nem kell saját render backendeket írnunk. A shaderek a bgfx `shaderc` eszközével fordulnak. A `shaderc` target konfigurálva van, de csak akkor épül, ha valami hivatkozik rá.
- **miniaudio:** egyetlen fájl, minden platformon fut, és van benne mixer, 3D spatialization és dekóder.
- **fastgltf:** gyors, modern C++ glTF 2.0 betöltő és exportáló. A glTF importer (és a `SampleModelWriter` teszt-`.glb`-je) erre épül.
- **ufbx:** egyfájlos (`ufbx.h` + `ufbx.c`) FBX betöltő tengely- és egységkonverzióval, skinnel és animáció-bake-eléssel. Saját `ufbx` statikus targetként fordul (SYSTEM include, warningok nem hibák), `PRIVATE`.
- **fast_obj:** egyfejléces OBJ/MTL olvasó. `INTERFACE` target SYSTEM include-dal; az implementáció egyetlen engine TU-ban (`src/Import/FastObjImplementation.cpp`) fordul, warning-elnyomó pragmák között, `PRIVATE`. A fájlokat saját callbackek olvassák (`Core::FileBytes`), így a nem ASCII Windows utak is működnek.
- **EnTT:** header-only, gyors, és ez a legelterjedtebb C++ ECS.
- **AngelScript:** C++-szerű, statikusan típusos scriptnyelv. Egyszerű benne natív típusokat regisztrálni, és nincs benne JIT, ezért mobilon és konzolon is használható. A buildbe ezek az add-onok kerülnek: `scriptarray`, `scriptbuilder`, `scriptdictionary`, `scripthandle`, `scripthelper`, `scriptmath`, `scriptstdstring`, `weakref`.
- **Jolt Physics:** modern, többszálú fizikai motor. A GPU compute backendek (DX12/Vulkan/Metal) ki vannak kapcsolva.
- **ozz-animation:** gyors, SoA alapú csontváz-animációs runtime (mintavételezés, keverés, local-to-model). Csak az `ozz_base`, `ozz_animation` és `ozz_animation_offline` könyvtárakat fordítjuk (eszközök, FBX/glTF konverterek, minták és tesztek nélkül), `PRIVATE`-ként linkelve. Az ozz saját `.ozz` archívumait nem használjuk: a runtime objektumok a Flawy `.flskel` / `.flclip` leíróiból, memóriában épülnek (lásd [animation.md](animation.md)).
- **Slang + DXC:** a shadereket Slang nyelven írjuk, és az engine futás közben fordítja őket minden bgfx backendre. A DXC a D3D12-höz szükséges DXIL kódot állítja elő. Részletek: [shaders.md](shaders.md).
- **GLM, Dear ImGui, Tracy, spdlog:** az alap kiegészítők a matekhoz, a debug UI-hoz, a profilinghoz és a logoláshoz.

## Később bekerülhet

- **meshoptimizer:** mesh-optimalizálás és tömörítés
- **Basis Universal / KTX2:** tömörített textúrák
- **Box2D v3:** ha lesz 2D fizika

## Új függőség felvétele

1. Vegyél fel egy sort a `scripts/dependencies.txt`-be (név, repo URL, tag vagy commit, submodule-ok kellenek-e).
2. Futtasd az install scriptet.
3. A `cmake/FlawyDependencies.cmake`-ben add hozzá a `FLAWY_REQUIRED_VENDORS` listához, és vedd fel a targetet.
4. Az `Flawy/CMakeLists.txt`-ben linkeld a `Flawy` targethez. `PUBLIC` legyen, ha a típusai megjelennek az engine publikus headereiben, egyébként `PRIVATE`.
5. Frissítsd ezt a dokumentumot.
