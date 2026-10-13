# Flawy

Egyszerű, multiplatform, code-only C++20 game engine.

## Projekt

- `Flawy/`: az engine lib (`Flawy::Flawy`)
- `Flawy-Sandbox/`: konzolos tesztalkalmazás

## Gyors kezdés

```powershell
scripts/install.bat
cmake --preset vs2026
cmake --build --preset vs2026-debug
./build/vs2026/bin/Debug/Flawy-Sandbox.exe
```

Az `install.bat` duplakattal is indítható. Linuxon és macOS-en a `./scripts/install.sh` szkriptet és a `ninja-debug` presetet használd.

## Dokumentáció

- [Toolset](docs/toolset.md)
- [Architektúra és projektstruktúra](docs/architecture.md)
- [Coding standards](docs/coding-standards.md)
- [Shader pipeline](docs/shaders.md)
