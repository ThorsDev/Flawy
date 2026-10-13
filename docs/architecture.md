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
│   ├── dependencies.txt      rögzített függőséglista
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
```
