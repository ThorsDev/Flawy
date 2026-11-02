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
| Assets | `Flawy::Assets` | asset-kezelés: `AssetManager`, loaderek/importerek, `.fl*` bináris formátumok, `StaticMeshAsset` (lásd [assets.md](assets.md)) |
| Animation | `Flawy::Animation` | csontváz, klipek, póz-mintavételezés és -keverés, animation controller és `Animator` (ozz-animation, pimpl mögött), `.flskel` / `.flclip` / `.flctrl` assetek (lásd [animation.md](animation.md)) |
| Import | `Flawy::Import` | modell-importerek (`ModelImporter`, `ModelImporterFactory`): glTF (fastgltf), FBX (ufbx), OBJ (fast_obj) → `StaticMeshAsset` / `SkeletonAsset` / `AnimationClipAsset`, opcionális mentéssel (lásd [assets.md](assets.md#import-gltf-fbx-obj)) |
| Scene | `Flawy::Scene` | ECS, entitások, komponensek (EnTT) |
| Physics | `Flawy::Physics` | fizika (Jolt) |
| Scripting | `Flawy::Scripting` | AngelScript integráció |

A modulok közti függőség egy irányba mutat: a Core-ra mindenki építhet, a Core viszont semmire. A harmadik féltől származó típusok lehetőleg ne szivárogjanak ki a publikus headerekbe. Ha mégis kell, a könyvtárat `PUBLIC`-ként kell linkelni.

**Kivétel: `Core::Application` a composition root.** Ez az egyetlen hely a Core-ban, amely a Platform, a Renderer, az Assets, az Animation és az Import modulra építhet: létrehozza az `IPlatformSystem`-et, az `IWindow`-t, a `RenderDevice`-t, a `MaterialSystem`-et, a `ShaderLibrary`-t, az `Assets::AssetManager`-t és a `RenderGraph`-ot, majd fordított sorrendben bontja le őket. A Core többi része (Logger, BuildInfo, ...) továbbra is függőségmentes.

### Application életciklus

- `Initialize()`: platform → ablak → `RenderDevice` (az ablak pixelméretével) → `MaterialSystem` (az `ApplicationSpecification::optShaderCacheDirectory`-val mint lemezes shader cache könyvtárral) → `ShaderLibrary` (a `MaterialSystem` compilerével és cache-ével, az `ApplicationSpecification::vShaderSearchRoots` keresési gyökerekkel) → `Assets::AssetManager` (az `ApplicationSpecification::optAssetRoot` asset roottal; az engine loaderei, a `StaticMeshLoader`, a `SkeletonLoader`, az `AnimationClipLoader` és az `AnimationControllerLoader`, valamint a glTF/FBX/OBJ importerek itt regisztrálódnak) → `RenderGraph`, majd `OnStart()`. Másodszori hívása no-op.
- `Tick()`: eventek feldolgozása (bezárás → kilépés, méretváltozás → `RenderDevice::Resize`), `OnUpdate(dt)`, `RenderGraph::BeginFrame()`, `OnRender(RenderGraph&)`, `RenderGraph::Execute()`, `RenderDevice::EndFrame()` (`bgfx::frame()`).
- `Shutdown()`: `OnShutdown()`, majd az `AssetManager` (a cache-elt assetekkel együtt, így a GPU-alapú assetek még élő eszközön szabadulnak fel), a `RenderGraph` (a transient target poollal együtt), a `ShaderLibrary`, a `MaterialSystem`, a renderer, az ablak és a platform lebontása. A bgfx handle-öket burkoló alosztály-erőforrásokat (bufferek, textúrák, `RenderTarget`-ek, `ShaderParameters`, `Material`-ok és `MaterialInstance`-ok) az `OnShutdown()`-ban kell felszabadítani; ide tartoznak a `ShaderLibrary` programjaira épülő `ShaderParameters` példányok is (magukat a programokat a `ShaderLibrary` birtokolja), valamint a GPU-alapú assetekre (pl. `StaticMeshAsset`) mutató `AssetHandle`-ök.
- `Run()` = `Initialize()` + `Tick()` ciklus amíg fut + `Shutdown()`. A `Tick()` és a `Shutdown()` `Initialize()` előtt `std::logic_error`-t dob.
- A bgfx egyszálú módban fut: a `RenderDevice` az `bgfx::init()` előtt egyszer meghívja a `bgfx::renderFrame()`-et a fő szálon.

## Render graph

A host nem lát bgfx view id-t: minden rajzolás egy `IRenderPass`-on keresztül megy, amit az `OnRender(RenderGraph&)` minden frame-ben hozzáad a graph-hoz.

- **Passok:** az `IRenderPass` tartós objektum. A `RenderGraph::AddPass` azonnal meghívja a `Setup(RenderPassBuilder&)`-et, ahol a pass deklarálja, mit ír (`WriteTarget`, pontosan egy target, `RenderPassClear`-rel) és mit olvas (`ReadTexture`). Az `Execute(RenderPassContext&)` a `RenderGraph::Execute()` alatt fut; itt a pass a `SetViewTransform`-mal és a `Draw(DrawCommand)`-dal rajzol, a deklarált olvasásokat a `GetTexture`-rel éri el. A `SetView` / `SetLights` a pass nézet- és fényadatait állítja (passonként nullázódnak). A `Setup`-ban a `SetMaterialPass` választja ki, melyik anyag-pass permutációval rajzolnak a pass anyagos `DrawCommand`-jai (alapértelmezés `MaterialPass::Forward`; lásd [shaders.md](shaders.md)).
- **Erőforrások:** a backbuffer (`GetBackbuffer()`), importált `RenderTarget`-ek (`ImportTarget`, a felhasználó birtokolja és méretezi, pl. egy editor viewport) és transient targetek (`CreateTransientTarget`, a graph poolja adja). A `RenderTargetDescriptor` 0-s szélessége/magassága a backbuffer méretét jelenti.
- **Handle-ök érvényessége:** a `RenderGraphResource` egy frame-sorszámot (generációt) hordoz. Az importált és transient handle-ök csak abban a frame-ben érvényesek, amelyben létrejöttek; egy korábbi frame handle-je `std::logic_error`-t ad. A backbuffer handle stabil generációjú (`kPersistentGeneration`), ezért frame-ek között is használható.
- **Builder és context élettartama:** a `RenderPassBuilder` csak a saját `Setup`-ján, a `RenderPassContext` csak a saját `Execute`-ján belül használható; máskor (megtartott referenciával) `std::logic_error`-t dobnak. Az `AddPass` `Setup`-ból vagy `Execute`-ból hívva szintén `std::logic_error`. Korlát: a builder és a context graph-onként egyetlen közös objektum, ezért egy megtartott referencia egy másik pass `Setup`-ja / `Execute`-ja alatt használva azon a passon hat, és ezt a graph nem ismeri fel.
- **Ellenőrzés:** egy pass csak olyan transient targetet olvashat, amelyet egy korábbi pass már írt (különben "reads ... before it is written"); a backbuffer nem olvasható textúraként; egy pass nem olvashatja azt, amit ír.
- **Culling:** a backbuffert vagy importált targetet író passok (és a `SetAlwaysExecute(true)` passok) élnek. Visszafelé haladva egy élő pass minden olvasott erőforrásának a legközelebbi korábbi íróját élővé teszi, és ha a pass nem törli a target minden meglévő aspektusát (szín; mélység és stencil, ha a targetnek van), akkor a saját targetjének legközelebbi korábbi íróját is. A többi pass nem fut, és a transient targetjük sem jön létre.
- **View-k:** az élő passok végrehajtási sorrendben kapják a 0..N-1 view id-t, minden frame-ben teljes view-állapottal (név, `Sequential` mód, frame buffer, rect, clear, transform). Egy view = egy pass = egy target. Ha egyik élő pass sem írja a backbuffert, egy további view feketére törli.
- **Transient pool:** a feloldott `RenderTargetDescriptor` a kulcs. Egy transient target első írója kapja a pool tartalmát, ami tetszőleges (egy korábbi frame vagy erőforrás maradéka); ha nem töröl (`RenderPassClear`), az eredmény nem definiált. Egy frame-en belül egy target csak egy transient erőforrást szolgál ki; a frame végén a nem használt targetek öregednek, és 4 nem használt frame után felszabadulnak (pl. ablakméret-változás után a régi méret). A létrehozást és a felszabadítást info szinten logolja.
- **Végső kimenet:** amit az utolsó pass ír. Backbufferbe vagy importált `RenderTarget`-be írni ugyanaz az API, így egy editor a jelenetet a saját targetjébe rendereli, és annak színtextúráját (`RenderTarget::GetColorTexture`) mintavételezi.

## Függőségek láthatósága

| Láthatóság | Könyvtárak | Miért |
|---|---|---|
| `PUBLIC` | GLM, EnTT, spdlog, Dear ImGui | a publikus API-ban is megjelenhetnek (matek típusok, registry, logger, debug UI egy editorhoz) |
| `PRIVATE` | SDL3, bgfx (bx, bimg), miniaudio, fastgltf, simdjson, AngelScript, Jolt, Tracy, Slang, ozz-animation, ufbx, fast_obj | implementációs részlet, csak az engine `.cpp` fájljai látják |

Mivel a lib statikus, a `PRIVATE` függőségek is automatikusan belinkelődnek a fogyasztóba, így a host oldalon nincs vele teendő.

## Build

A minimális CMake verzió 3.30 (az ozz-animation ezt követeli meg); a `CMakeLists.txt` és a `CMakePresets.json` is ezt deklarálja.

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
