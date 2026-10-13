# Flawy – Shader pipeline (terv)

> Állapot: az 1. fázis elkészült (Vulkan és D3D11, runtime fordítás, egy default shader, háromszög a Sandboxban).

## Célok

- A shadereket **Slang** nyelven írjuk.
- **Minden bgfx backendet** támogatunk: D3D11, D3D12, Vulkan, Metal, OpenGL, OpenGL ES és WebGPU.
- A **default shaderek forrásként** vannak a repóban, nem előre fordított binárisként.
- Az **engine maga fordítja a shadereket** futás közben. Így később a felhasználók saját shadereket is írhatnak, és ezeket az engine ugyanazzal a pipeline-nal fordítja le.
- Az engine lib továbbra is önálló: egy hostnak (editornak, játéknak) csak a `Flawy::Flawy`-t kell linkelnie, a default shaderek is benne vannak.

## Felépítés

### Default shaderek

```
Flawy/
├── shaders/              default Slang shaderek forrásként (.slang)
│   ├── Common/           közös modulok (pl. kamera, transzformációk)
│   └── ...
```

A build a `Flawy/shaders/` tartalmát **beágyazza a libbe**. Egy CMake lépés generált forrásfájlt készít belőlük. Így a host alkalmazásnak nem kell külön fájlokat másolnia, és az editor is eléri a defaultokat.

### Runtime komponensek (`Flawy::Renderer` modul)

| Osztály | Felelősség |
|---|---|
| `ShaderSource` | egy shader forrása, entry pointjai, define-jai és a modulkeresési útvonalak |
| `IShaderSourceProvider` | honnan jön a forrás. Implementációi: `EmbeddedShaderSourceProvider` (a beágyazott defaultok) és `FileShaderSourceProvider` (custom shaderek lemezről) |
| `ShaderCompiler` | Slang session kezelése, a forrás lefordítása az **aktív** renderer backendre, és a reflexiós adatok kiolvasása |
| `IShaderBackendEmitter` | backendenként egy implementáció, amely a Slang kimenetet a bgfx elvárásaira alakítja (lásd a táblázatot lent) |
| `BgfxShaderInterfaceBuilder` | a Slang reflexióból felépíti a bgfx uniform- és attribútumlistáját, és ellenőrzi, hogy a shader belefér-e a bgfx-profilba |
| `BgfxShaderBinaryWriter` | megírja a bgfx `.bin` shaderformátumot (jelenleg a 12-es verziót) |
| `ShaderCache` | a lefordított binárisok tárolása a forrás, a define-ok, a backend és a compiler verziója alapján képzett kulccsal. Memóriában és lemezen is tárol |
| `ShaderLibrary` | a magas szintű API: név alapján shader programot ad vissza, közben a cache-t vagy a compilert használja. Itt kap helyet később a hot reload is |

Futás közben csak az **aktív** backendre fordítunk, amit a `bgfx::getRendererType()` ad meg. Az összes backendre egyszerre csak offline "bake" módban fordítunk.

### Backendenkénti útvonal

| Backend | A bgfx ezt várja | Útvonal |
|---|---|---|
| Vulkan | SPIR-V | Slang → SPIR-V, a bindingok a bgfx konvenciója szerint |
| D3D12 | DXIL | Slang → DXIL (a DXC dinamikusan töltődik be) |
| D3D11 | DXBC | Slang → DXBC (fxc / `d3dcompiler_47.dll`, csak Windowson) |
| Metal | MSL forráskód | Slang → SPIR-V → SPIRV-Cross → MSL |
| OpenGL / GLES | GLSL / ESSL, bgfx-es elnevezésekkel | Slang → SPIR-V → SPIRV-Cross (flattenelt uniformokkal és átnevezéssel) |
| WebGPU | WGSL | Slang → WGSL |

A SPIRV-Cross már most is a bgfx `3rdparty` mappájában van.

## A bgfx-profil (a shaderekre vonatkozó megkötések)

A bgfx uniform-modellje miatt minden shadernek tartania kell magát ezekhez:

- A uniformok különálló `float4`, `float3x3`, `float4x4` értékek, vagy ezek tömbjei. Lehetnek még textúrák és samplerek. Más típusú uniform nem megengedett: nincs int, és nincs tetszőleges struct a uniformok között.
- A vertex inputok a `bgfx::Attrib` szerinti szemantikákat használják (`POSITION`, `NORMAL`, `TEXCOORD0..7`, `COLOR0..3` stb.).
- Compute shaderekben a storage bufferek és az image-ek a bgfx binding modellje szerint használhatók.

Ha egy shader ezt megszegi, a `BgfxShaderInterfaceBuilder` fordításkor érthető hibát ad.

## Platform-elérhetőség

| Platform | Futás közbeni fordítás | Megoldás |
|---|---|---|
| Windows, Linux, macOS | igen (a Slang dinamikus libként jön) | runtime fordítás és lemezes cache |
| iOS, Android, Web | nem (nincs hozzá Slang runtime) | offline bake: a desktop engine minden backendre lefordít, és a játék a kész cache-t kapja meg |

Ugyanaz a `ShaderCompiler` szolgálja ki mindkét esetet. A bake csak egy másik belépési pont (Sandbox parancs vagy későbbi CLI).

## Függőségek

| Függőség | Szerep | Licenc |
|---|---|---|
| Slang | frontend és kódgenerálás | Apache-2.0 WITH LLVM-exception |
| SPIRV-Cross | MSL és GLSL/ESSL előállítása | Apache-2.0 (a bgfx 3rdparty mappájából) |
| DXC (`dxcompiler`, `dxil`) | DXIL előállítása D3D12-höz | NCSA / MIT |

A Slang dinamikus libként töltődik be (`slang.dll` / `libslang.so` / `libslang.dylib`). A build a futtatható fájl mellé másolja.

## Az 1. fázis állapota

Elkészült:

- `ShaderCompiler`: Slang runtime fordítás az aktív backendre. Vulkan: SPIR-V (`spirv_1_3`), D3D11: DXBC (`sm_5_0`, a rendszer `d3dcompiler_47.dll`-jén keresztül).
- `BgfxShaderInterfaceBuilder` és `BgfxShaderBinaryWriter`: bgfx shader bináris (12-es verzió), uniformlistával, attribútumtáblával és uniform bufferméret-adattal.
- `EmbeddedShaderSourceProvider`: a `Flawy/shaders/*.slang` fájlokat a build egy generált `EmbeddedShaderTable.cpp`-be ágyazza (`cmake/FlawyEmbedShaders.cmake`). A `.slang` fájl módosítása után a build újragenerálja a táblát.
- `ShaderProgram`, `VertexLayout`, `VertexBuffer`, `RenderDevice::SetViewTransform` / `Submit`.
- Default shader: `Unlit.slang` (`VertexMain`, `FragmentMain`), a bgfx előre definiált `u_modelViewProj` uniformjával.

Uniform binding konvenciók:

- Vulkan: a globális uniform blokk a vertex shaderben `set 0, binding 0`, a fragment shaderben `set 0, binding 1` (bgfx `kSpirvVertexBinding` / `kSpirvFragmentBinding`). Ezt a `VulkanBindGlobals` opció állítja be.
- D3D11: a globális uniform blokk mindkét stage-ben `b0`.
- A uniformok `regIndex` mezője a bájt-offset a blokkon belül, a méret 16 bájtra kerekítve kerül a binárisba.

Ellenőrzött tények és korlátok:

- **Runtime DLL-ek:** csak a `slang.dll` és a `slang-compiler.dll` kell. A build a `flawy_copy_runtime_dependencies` segítségével a futtatható fájl mellé másolja őket. A `slang-glslang.dll`, `slang-glsl-module.dll`, `slang-llvm.dll`, `slang-rt.dll`, `gfx.dll` és a standard module mappa nem szükséges.
- **Textúrák és samplerek:** az 1. fázisban a `BgfxShaderInterfaceBuilder` érthető hibával elutasítja őket ("textures are supported from Phase 2").
- **Reflexió vs. `VulkanBindGlobals`:** a Slang reflexió (`getGlobalConstantBufferBinding`) nem veszi figyelembe a `VulkanBindGlobals` felülírást, a generált SPIR-V viszont igen. Ezért Vulkanon a binding ellenőrzése a generált SPIR-V `Binding` / `DescriptorSet` dekorációin történik, D3D11-en a reflexión.
- **Stage-enkénti uniform lista:** a uniform blokk használatát a Slang metadata (`isParameterLocationUsed`) blokk szinten adja meg. Ha egy stage használja a blokkot, a modul összes globális uniformja bekerül a stage uniformlistájába. Ez a bgfx számára helyes, csak nem minimális.
- **`VertexLayout::GetStride`:** a bgfx D3D/Vulkan mérettáblázatát követi. GL-en és Metalon a 3 komponensű `Uint8` / `Int16` / `Half` attribútumok mérete eltér, ezért ezeknek a backendeknek a felvétele (3. fázis) előtt vagy a `VertexLayout::Add` utasítsa el ezeket a kombinációkat, vagy a stride a feloldott bgfx layoutból jöjjön.
- **`-fvk-t-shift` / `-fvk-s-shift`:** a slangc 2026.19 ezekkel összeomlik. A 2. fázis textúra bindingjaihoz SPIR-V binding-átíró utófeldolgozás kell.

## Fázisok

1. Slang integráció, `ShaderCompiler`, `BgfxShaderBinaryWriter`, Vulkan és D3D11 backend, egy default shader, és egy háromszög a Sandboxban.
2. A `ShaderCache` és a `ShaderLibrary`, valamint a custom shaderek fájlból.
3. D3D12, Metal, OpenGL/GLES.
4. WebGPU, offline bake mód, hot reload.

## Döntések

- **Slang:** az install script a GitHub release-ből tölti le a platformhoz illő előre fordított csomagot (v2026.19).
- **DXC:** szintén előre fordított GitHub release-ből jön (v1.9.2609), Windowson és Linuxon.
