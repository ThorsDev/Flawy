# Flawy – Architektúra és projektstruktúra

## Alapelvek

- **Code-only engine.** Nincs beépített vizuális editor. Az engine egy könyvtár, amit kódból használunk.
- **Az engine egyetlen önálló lib (`Flawy::Flawy`).** Egy új host alkalmazásnak (sandbox, később egy vizuális editor vagy egy játék) csak ezt az egy targetet kell linkelnie. Minden függőséget a lib hoz magával.
- **OOP, clean code.** Kis, egyetlen felelősségű osztályokra bontunk, a függőségek interfészek mentén mennek, RAII-t használunk, és a kódban nincsenek kommentek. A részletek a [coding-standards.md](coding-standards.md)-ben vannak.

## Mappastruktúra

```
./ (repo gyökér)
├── CMakeLists.txt            gyökér CMake projekt
├── CMakePresets.json         build presetek (vs2026, ninja-debug, ninja-release)
├── CLAUDE.md                 Claude Code projektszabályok és agent workflow
├── cmake/
│   ├── FlawyCompilerOptions.cmake   közös warning / fordító beállítások
│   └── FlawyDependencies.cmake      vendor targetek felvétele és konfigurálása
├── docs/                     dokumentáció
├── Flawy/                    a Flawy engine lib (target: Flawy::Flawy)
│   ├── include/Flawy/        publikus API (modulonként almappák: Core, Platform, Renderer, ...)
│   └── src/                  implementáció, ugyanazzal a modulstruktúrával
├── Flawy-Sandbox/            konzolos tesztalkalmazás (target: Flawy-Sandbox)
│   └── src/
├── scripts/
│   ├── dependencies.txt      rögzített, gitről klónozott függőségek
│   ├── prebuilt.txt          rögzített, előre fordított release-ek platformonként (Slang, DXC)
│   ├── install.bat           Windows launcher (duplakattal is futtatható), az install.ps1-et hívja
│   ├── install.ps1           függőségek letöltése (Windows)
│   └── install.sh            függőségek letöltése (Linux/macOS)
├── vendor/                   letöltött függőségek (gitignored)
└── build/                    build kimenet (gitignored)
```

## Modulok

Minden modul saját namespace-t és almappát kap:

| Modul | Namespace | Felelősség |
|---|---|---|
| Core | `Flawy::Core` | Application életciklus, logolás, build info, időzítés |
| Platform | `Flawy::Platform` | ablak, input, eventek (SDL3) |
| Renderer | `Flawy::Renderer` | renderelés (bgfx) |
| Audio | `Flawy::Audio` | hang (miniaudio) |
| Assets | `Flawy::Assets` | modellek, textúrák, asset-kezelés (fastgltf) |
| Scene | `Flawy::Scene` | ECS, entitások, komponensek (EnTT) |
| Physics | `Flawy::Physics` | fizika (Jolt) |
| Scripting | `Flawy::Scripting` | AngelScript integráció |

A modulok közti függőség egy irányba mutat: a Core-ra mindenki építhet, a Core viszont semmire. A harmadik féltől származó típusok lehetőleg ne szivárogjanak ki a publikus headerekbe. Ha mégis kell, a könyvtárat `PUBLIC`-ként kell linkelni.

**Kivétel: `Core::Application` a composition root.** Ez az egyetlen hely a Core-ban, amely a Platform és a Renderer modulra építhet: létrehozza az `IPlatformSystem`-et, az `IWindow`-t és a `RenderDevice`-t, majd fordított sorrendben bontja le őket. A Core többi része (Logger, BuildInfo, ...) továbbra is függőségmentes.

### Application életciklus

- `Initialize()`: platform → ablak → `RenderDevice` (az ablak pixelméretével), majd `OnStart()`. Másodszori hívása no-op.
- `Tick()`: eventek feldolgozása (bezárás → kilépés, méretváltozás → `RenderDevice::Resize`), `OnUpdate(dt)`, `BeginFrame()`, `OnRender()`, `EndFrame()`.
- `Shutdown()`: `OnShutdown()`, majd a renderer, az ablak és a platform lebontása. A bgfx handle-öket burkoló alosztály-erőforrásokat az `OnShutdown()`-ban kell felszabadítani.
- `Run()` = `Initialize()` + `Tick()` ciklus amíg fut + `Shutdown()`. A `Tick()` és a `Shutdown()` `Initialize()` előtt `std::logic_error`-t dob.
- A bgfx egyszálú módban fut: a `RenderDevice` az `bgfx::init()` előtt egyszer meghívja a `bgfx::renderFrame()`-et a fő szálon.

## Függőségek láthatósága

| Láthatóság | Könyvtárak | Miért |
|---|---|---|
| `PUBLIC` | GLM, EnTT, spdlog, Dear ImGui | a publikus API-ban is megjelenhetnek (matek típusok, registry, logger, debug UI egy editorhoz) |
| `PRIVATE` | SDL3, bgfx, miniaudio, fastgltf, AngelScript, Jolt, Tracy | implementációs részlet, csak az engine `.cpp` fájljai látják |

Mivel a lib statikus, a `PRIVATE` függőségek is automatikusan belinkelődnek a fogyasztóba, így a host oldalon nincs vele teendő.

## Build

```powershell
scripts/install.bat
cmake --preset vs2026
cmake --build --preset vs2026-debug
./build/vs2026/bin/Debug/Flawy-Sandbox.exe
```

Linuxon és macOS-en:

```bash
./scripts/install.sh
cmake --preset ninja-debug
cmake --build --preset ninja-debug
./build/ninja-debug/bin/Flawy-Sandbox
```

CMake opciók:

- `FLAWY_BUILD_SANDBOX`: alapból csak akkor kapcsol be, ha a Flawy a top-level projekt
- `FLAWY_WARNINGS_AS_ERRORS`: alapból `ON`
- `FLAWY_ENABLE_PROFILING`: a Tracy kapcsolója, alapból `OFF`

## Beágyazás új környezetbe

```cmake
add_subdirectory(path/to/Flawy)
target_link_libraries(MyEditor PRIVATE Flawy::Flawy)
flawy_copy_runtime_dependencies(MyEditor)
```

A `flawy_copy_runtime_dependencies(<target>)` Windowson egy POST_BUILD lépésben a `$<TARGET_RUNTIME_DLLS>` által visszaadott DLL-eket (pl. a Slang runtime-ot) a futtatható állomány mellé másolja; üres lista esetén no-op. A `flawy_configure_target` minden `EXECUTABLE` targetre automatikusan meghívja, így a repón belüli hostoknál (Flawy-Sandbox) nincs vele teendő.
