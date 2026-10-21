# Flawy – Shader pipeline (terv)

> Állapot: az 1. fázis elkészült (Vulkan és D3D11, runtime fordítás, default shaderek). Az engine 2. fázisában elkészültek a textúrák és samplerek (`Flawy/Sampler.slang`), a `ShaderParameters` és a render graph. Az engine 3. fázisában elkészült az anyagrendszer alapja (3a: Slang generikus specializáció, `Material` / `MaterialInstance`, anyagos `DrawCommand`) a megvilágítás (3b: nézet- és fényadatok, Lit PBR, tangenses vertex factory, sRGB textúrák, HDR jelenet + tonemap) és az anyaggráf (3c: `MaterialGraph`, JSON, `MaterialCompiler`).

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
| `ShaderCache` | (kész) a specializált (anyag-) shaderek `CompiledShader` eredményeinek lemezes tárolása, a fordítás bemeneteiből és a compiler ujjlenyomatából számolt 128 bites kulccsal (lásd [Shader cache](#shader-cache)) |
| `ShaderLibrary` | (tervezett) a magas szintű API a nem anyag-shaderekhez (post-process, custom shaderek fájlból): név alapján `ShaderProgram`-ot ad vissza, a fordításhoz a `ShaderCompiler`-t, a tároláshoz a `ShaderCache`-t használja. Itt kap helyet később a hot reload is |

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
- `ShaderProgram` (összefésült uniformlista, `FindUniform`), `ShaderParameters`, `VertexLayout`, `VertexBuffer`, `IndexBuffer`, `Texture`, `RenderTarget`. A rajzolás a render graph-on keresztül megy: `RenderPassContext::SetViewTransform` / `Draw` (lásd [architecture.md](architecture.md)).
- Default shaderek (`VertexMain`, `FragmentMain`): `Unlit.slang` (a bgfx előre definiált `u_modelViewProj` uniformjával), `Textured.slang` (textúra × vertex szín × `u_tint`), `PostProcess.slang` (teljes képernyős quad, `u_tint.rgb` színezés és `u_tint.a` erősségű vignetta). Közös include: `Flawy/Sampler.slang`.

Uniform binding konvenciók:

- Vulkan: a globális uniform blokk a vertex shaderben `set 0, binding 0`, a fragment shaderben `set 0, binding 1` (bgfx `kSpirvVertexBinding` / `kSpirvFragmentBinding`). Ezt a `VulkanBindGlobals` opció állítja be.
- D3D11: a globális uniform blokk mindkét stage-ben `b0`.
- A uniformok `regIndex` mezője a bájt-offset a blokkon belül, a méret 16 bájtra kerekítve kerül a binárisba.

Ellenőrzött tények és korlátok:

- **Runtime DLL-ek:** csak a `slang.dll` és a `slang-compiler.dll` kell. A build a `flawy_copy_runtime_dependencies` segítségével a futtatható fájl mellé másolja őket. A `slang-glslang.dll`, `slang-glsl-module.dll`, `slang-llvm.dll`, `slang-rt.dll`, `gfx.dll` és a standard module mappa nem szükséges.
- **Reflexió vs. `VulkanBindGlobals`:** a Slang reflexió (`getGlobalConstantBufferBinding`) nem veszi figyelembe a `VulkanBindGlobals` felülírást, a generált SPIR-V viszont igen. Ezért Vulkanon a binding ellenőrzése a generált SPIR-V `Binding` / `DescriptorSet` dekorációin történik, D3D11-en a reflexión.
- **Stage-enkénti uniform lista:** a uniform blokk használatát a Slang metadata (`isParameterLocationUsed`) blokk szinten adja meg. Ha egy stage használja a blokkot, a modul összes globális uniformja bekerül a stage uniformlistájába. Ez a bgfx számára helyes, csak nem minimális.
- **`VertexLayout::GetStride`:** a bgfx D3D/Vulkan mérettáblázatát követi. GL-en és Metalon a 3 komponensű `Uint8` / `Int16` / `Half` attribútumok mérete eltér, ezért ezeknek a backendeknek a felvétele (3. fázis) előtt vagy a `VertexLayout::Add` utasítsa el ezeket a kombinációkat, vagy a stride a feloldott bgfx layoutból jöjjön.
- **`-fvk-t-shift` / `-fvk-s-shift`:** a slangc 2026.19 ezekkel összeomlik. Nincs rájuk szükség: a textúrák bindingját a `FLAWY_SAMPLER2D` makró explicit `vk::binding` és `register` attribútumokkal rögzíti (lásd lent).

## Textúrák és samplerek

A bgfx Vulkanon külön textúra- és sampler-descriptort vár, ezért a Slang kombinált `Sampler2D` típusa nem használható. A shaderek a beágyazott include-ot használják:

```slang
#include "Flawy/Sampler.slang"

FLAWY_SAMPLER2D(s_texColor, 0);

float4 color = FLAWY_SAMPLE2D(s_texColor, uv);
```

- A `FLAWY_SAMPLER2D(name, stage)` egy `Texture2D name` és egy `SamplerState nameSampler` deklarációt ad. Vulkan: `binding 2 + stage` és `18 + stage` (set 0; bgfx `kSpirvBindShift` / `kSpirvSamplerShift`). D3D11: `register(t<stage>)` és `register(s<stage>)`.
- Az `#include` a `ShaderCompiler` saját `ISlangFileSystem`-én (`ShaderSourceFileSystem`) keresztül a beágyazott shadertáblából oldódik fel (`\` → `/`, a vezető `./` levágva).
- A `BgfxShaderInterfaceBuilder` szabályai: csak írásvédett (read-only) `Texture2D`; minden textúrához kell egy `<név>Sampler` sampler a textúra bindingja + 16 (Vulkan) / azonos regiszter (D3D11) helyen; páratlan sampler, ütköző stage, 16 feletti stage és 0-tól eltérő register space / descriptor set hiba. A stage-ben nem használt textúra (Slang metadata) kimarad az adott stage uniformlistájából.
- A binárisban a sampler uniform típusbájtja `Sampler | 0x20` (fragment shaderben `| 0x10` is), `regIndex` = Vulkan binding / D3D regiszter, `texDimension = 0x02` (2D). A publikus `ShaderUniform::uTextureStage` adja a stage-et.
- A `ShaderProgram` elutasítja, ha a vertex és a fragment shader ugyanazt a samplert más stage-en, vagy két különböző samplert ugyanazon a stage-en deklarál.
- Értékek beállítása: `ShaderParameters` (a program uniformlistájából épül, `SetVec4` / `SetMat3` / `SetMat4` / tömbös változatok / `SetTexture`). Egy programhoz több `ShaderParameters` is tartozhat (anyagok). Rajzoláskor a `DrawCommand::pParameters` kötelező, ha a program samplert használ, és minden sampler slotba textúrát kell állítani; különben `std::logic_error`.
- UV-origó: a `PostProcess.slang` teljes képernyős quadja abból indul ki, hogy a render targetek origója a bal felső sarok (D3D11 és Vulkan), ezért a `TEXCOORD0` (0,0) a quad bal felső csúcsa, flip nélkül. GL és Metal (3. fázis) esetén a `bgfx::getCaps()->originBottomLeft` alapján fordítani kell.
- Konvenció: a bgfx a uniformokat név szerint osztja meg, ezért egy adott nevű uniform (pl. `u_tint`) minden shaderben ugyanolyan típusú legyen.

## Anyagrendszer (material system)

Egy anyag (material) egy kis Slang modul, amely egy `public struct <Név> : IMaterial` típust deklarál. A végső vertex/fragment programot az engine állítja elő: a pass shaderek generikus entry pointjait Slang generikus specializációval (`IComponentType::specialize`) példányosítja a (pass, vertex factory, anyag) hármasra. Nincs preprocesszoros permutáció és nincs generált "ragasztó" modul; a kézzel írt és a gráfból generált anyagok ugyanúgy működnek.

### Slang modulok

| Fájl | Tartalom |
|---|---|
| `Flawy/MaterialInterface.slang` | `ShadingModel` (`Unlit`, `Lit`), `SurfaceInput` (világpozíció, -normál, -tangens, nézetirány, UV0, vertex szín, idő), `SurfaceOutput`, `IMaterial` (`static const kShadingModel`, `static const kUsesTangentSpaceNormal`, `static EvaluateSurface`, `static EvaluateWorldPositionOffset`), `MakeDefaultSurface()` |
| `Flawy/VertexFactory.slang` | `VertexAttributes`, `IVertexFactory` (`associatedtype Input`, `kHasTangent`, `Fetch`), `StaticMeshVertexFactory` (`POSITION` 3, `NORMAL` 3, `TEXCOORD0` 2, `COLOR0` 4) és `StaticMeshTangentVertexFactory` (+ `TANGENT` 4, `w` = bitangens előjel) |
| `Flawy/View.slang` | `u_viewParams[2]` (kamera pozíció + idő, viewport méret és reciproka) és `u_lightParams[12]` (irányfény iránya + engedélyezés, irányfény radiancia, ég- és talaj-ambiens, 4 pontfény: pozíció + sugár, radiancia), elérő függvényekkel (`GetCameraPosition`, `GetTimeSeconds`, `GetPointLightRadiance(i)`, ...). Anyagok is importálhatják |
| `Flawy/Lighting.slang` | `ShadeLit`: Cook-Torrance (GGX eloszlás, magasság-korrelált Smith láthatóság, Schlick Fresnel, Lambert diffúz; `F0 = lerp(0.04, baseColor, metallic)`, érdesség `[0.045, 1]`), egy irányfény, legfeljebb 4 pontfény ablakolt inverz-négyzetes csillapítással (`saturate(1 - (d/r)^4)^2 / (d^2 + 1)`), féltér-ambiens (`lerp(ground, sky, n.y * 0.5 + 0.5)`), plusz emisszió. Árnyék nincs |
| `Flawy/PassTransform.slang` | `module PassTransform;` a pass shaderek közös vertex-transzformációja: `public uniform float4x4 u_model, u_viewProj`, `TransformPassVertex<M : IMaterial>(position, normal)` → világpozíció (WPO-val eltolva), világnormál és klip-pozíció. A világpozíció és a klip-pozíció `precise`, így a `Forward` és a `DepthOnly` bitre azonos mélységet ír (a `LessEqual` mélységteszt a prepass után megbízható) |
| `Flawy/Passes/DepthOnly.slang` | `module DepthOnly;` ugyanazok a generikus entry pointok; a VS a `TransformPassVertex`-szel (a Forwarddal azonos módon) számolja a (WPO-val eltolt) klip-pozíciót, és csak `SV_Position`-t ad ki; az FS konstans `float4(0, 0, 0, 1)` (a bgfx-nek kell fragment shader, a pass a `RenderState::bColorWrite = false`-szal rajzol). Az anyag `EvaluateSurface`-e nem fut, ezért a felületi textúrák és paraméterek kioptimalizálódnak |
| `Flawy/Passes/Forward.slang` | `module Forward;` `VertexMain<V : IVertexFactory, M : IMaterial>(V.Input)` és `FragmentMain<M : IMaterial>(VertexOutput)`. A klip-pozíció a `PassTransform` `TransformPassVertex`-éből jön (világpozíció a WPO-val eltolva, az előre definiált `u_viewProj`-jal); a nézetirány a `u_viewParams` kamera pozíciójából. Unlit: `baseColor + emissive`; Lit: `ShadeLit`. Tangens-térbeli normál csak `kUsesTangentSpaceNormal` esetén: a `kUsesTangentSpaceNormal` ág statikusan fordul ki, a tangens jelenlétét viszont futás közben ellenőrzi (`length(worldTangent.xyz) > 0`; tangens nélküli vertex factory esetén a VS nulla tangenst ír, így a vertex normál marad). A bitangens előjele tükröző (negatív determinánsú) `u_model` esetén megfordul. A normálok transzformációja `u_model`-lel történik, ezért nem egyenletes skálázás mellett pontatlan |

Az anyagoknak nincs példányértékük a shaderben: minden követelmény `static`, így a `M.kShadingModel` / `V.kHasTangent` fordításkor kiértékelődik. Az anyag állapota modulszintű uniform: `uniform float4 u_materialParams[N];` (vec4 slotok, a komponensek tömörítve) és `FLAWY_SAMPLER2D(s_materialTexture<i>, i);`.

A Sandbox jelenete gráfból generált anyagot használ (lásd lent). Kézzel írt példák: `Flawy-Sandbox/shaders/LitChecker.slang` (Lit) és `Flawy-Sandbox/shaders/CheckerUnlit.slang` (Unlit); a Sandbox egyiket sem tölti be, a konzolos probe-ok és a dokumentáció példái:

```slang
module CheckerUnlit;

import Flawy.MaterialInterface;
#include "Flawy/Sampler.slang"

uniform float4 u_materialParams[1];
FLAWY_SAMPLER2D(s_materialTexture0, 0);

public struct CheckerUnlit : IMaterial
{
    static const ShadingModel kShadingModel = ShadingModel::Unlit;
    static const bool kUsesTangentSpaceNormal = false;

    static SurfaceOutput EvaluateSurface(SurfaceInput oInput) { ... }
    static float3 EvaluateWorldPositionOffset(float3 vecWorldPosition, float3 vecWorldNormal) { return float3(0.0, 0.0, 0.0); }
}
```

Névszabályok: egy stringből betöltött modul a `ShaderModuleSource::sName` néven regisztrálódik, ennek egyeznie kell a `module <Név>;` deklarációval (a `ShaderCompiler` ellenőrzi), anyagoknál a struct nevével is. A pass modulok rövid névvel (`Forward`) töltődnek be, az engine könyvtármoduljai csak `import Flawy.<Név>;` formában érhetők el (`Flawy/<Név>` néven regisztrálódnak). Az `import` és az `#include` a beágyazott shadertáblából oldódik fel (először az importáló fájlhoz relatívan, aztán a gyökérből).

### Specializált fordítás

`ShaderCompiler::CompileSpecialized(SpecializedShaderRequest)`: a request a pass modul forrását, az entry pointot, a stage-et, a további modulokat (az anyagot) és a típusargumentumokat tartalmazza (VS: `{"StaticMeshVertexFactory", "<Anyag>"}`, FS: `{"<Anyag>"}`).

- Compilerenként egy tartós Slang session készül két targettel (0 = vertex, 1 = fragment; Vulkanon a globális uniform blokk binding 0 / 1, D3D11-en a két target azonos). A nyers `Compile` útvonal változatlanul hívásonként új sessiont használ.
- A modulok név szerint regisztrálódnak; ugyanaz a név ugyanazzal a forrással újrahasznosul, más forrással `std::invalid_argument` (hot reload nincs). Egy betöltött modul a session élettartamáig bent marad.
- Lépések: modulok betöltése → `findEntryPointByName` → kompozit `[pass, entry point, modulok...]` → `findTypeByName` minden típusargumentumra → `specialize` → `link` → a közös kibocsátó lépés (reflexió, metadata, `BgfxShaderInterfaceBuilder`, emitter, `BgfxShaderBinaryWriter`).
- A `CompiledShader::sName` `"<pass>::<entry><típusargumentumok>"`, pl. `Forward::VertexMain<StaticMeshVertexFactory,CheckerUnlit>`.
- Hibák `ShaderCompileError`-ként, a Slang diagnosztikával: hiányzó interfésztag (`does not provide required interface member`, már a modul betöltésekor), nem megfelelő típusargumentum (`doesn't conform`), ismeretlen típus (`Specialization type 'X' was not found in the loaded modules`), rossz típusargumentum-szám.
- Az Unlit anyag fragment shadere a statikus `kShadingModel` ág miatt nem tartalmazza a megvilágítást (a probe szerint D3D11-en 1552 vs. 8648, Vulkanon 2316 vs. 12084 bájt).
- Reflexió a specializáció után: egyetlen globális konstans buffer a pass modul, az anyag modul és az importált modulok (pl. `PassTransform`) globáljaival (a sorrend a Slang modulsorrendjét követi, a programok név szerint kötnek); a stage uniformlistájában (a korábbi szabály szerint) az összes globális szerepel, a textúrák közül csak a használtak.

### C++ API

| Típus | Felelősség |
|---|---|
| `MaterialParameterLayout` | a paraméterek alakja: értékparaméterek (`Float1..4`, slot + komponens, alapérték) és textúraparaméterek (stage); `Validate()` (`std::invalid_argument`: ismétlődő név, átlógó/átfedő komponens, slot a `uVec4SlotCount`-on túl, több mint 32 slot, ismétlődő vagy 16 feletti stage) |
| `MaterialDescriptor` | modulnév, Slang forrás, opcionális `sSourcePath` (ha meg van adva, ez a Slang modul útvonala, így a diagnosztika a valódi fájlra mutat; különben a virtuális `Materials/<Név>.slang`), paraméter-layout, debug név |
| `MaterialPass`, `VertexFactoryKind`, `MaterialPermutationKey` | a permutáció kulcsa: pass (`Forward`, `DepthOnly`), vertex factory (`StaticMesh`, `StaticMeshTangent`) és a static switch készlet indexe (`uStaticSwitchSet`, 0 = alapértékek) |
| `MaterialSystem` | anyagonként közös erőforrások: a backendhez tartozó `ShaderCompiler` (tartós specializációs sessionnel), a `ShaderCache` (`GetShaderCache()`), 1×1 fehér és lapos normál textúra. A `MaterialSystemSpecification::optShaderCacheDirectory` adja a lemezes cache könyvtárát (üresen a cache ki van kapcsolva). Az `Application` birtokolja (`GetMaterialSystem()`; a könyvtár az `ApplicationSpecification::optShaderCacheDirectory`-ból jön) |
| `Material` | a leíró ellenőrzése; `CompilePermutation(key)` lustán fordít és gyorsítótáraz: VS + FS kérés, mindkettő előbb a `ShaderCache`-ben, csak hiány esetén `CompileSpecialized` (az eredmény a cache-be kerül), `ShaderProgram`, majd a layout ellenőrzése a reflektált programon (ha a program használja az `u_materialParams`-t, az Vec4 legyen legalább `uVec4SlotCount` elemmel; minden `s_materialTexture<i>` sampler az `i` stage-en; eltérés esetén `std::logic_error` az anyag és a uniform nevével). A layoutban szereplő, de a permutációból kioptimalizált textúra vagy `u_materialParams` nem hiba (a `DepthOnly` pass és a static switch-ek kikapcsolt ágai ilyenek); fordítva viszont a programban lévő, layout nélküli `s_materialTexture*` sampler `std::logic_error`. A `u_viewParams` / `u_lightParams` reflektált mérete egyezzen az engine konstansaival (2 / 12). A sikeres fordítást info szinten logolja időméréssel, `(compiled)` / `(cache)` / `(partially cached)` jelöléssel és a switch értékekkel (pl. `permutation [DepthOnly, StaticMesh, set1] in 17.8 ms (cache); static switches: UseNormalMap=false`), a stage-enkénti uniformlistát debug szinten. Static switch-ek: `ResolveStaticSwitchSet(values)` a megadott (név, érték) párokat az alapértékekre teszi rá, és a teljes készlet indexét adja (meglévő készletet újrahasznosít, újat a `MaterialCompiler`-rel generál; ismeretlen név `std::invalid_argument`, leíróból készült anyagnál nem üres lista `std::logic_error`); `GetStaticSwitchNames()` név szerint rendezve, `GetStaticSwitchSetCount()` |
| `MaterialInstance` | CPU oldali értéktár (`SetFloat` / `SetVector` / `SetTexture` / `ClearTexture`, `Find*` / `Require*`); permutációnként lustán létrehozott `ShaderParameters`, amelyet első használatkor és minden módosítás után szinkronizál. Be nem állított textúra helyett a fehér textúra kerül a slotba. `SetFloat` csak `Float1`-re, `SetVector` csak `Float2..4`-re hívható. `SetStaticSwitch(name, value)` csak gráfból készült anyagnál (különben `std::logic_error`; ismeretlen név `std::invalid_argument`, ilyenkor a példány nem változik): újraoldja a példány teljes switch készletét, és a következő rajzolás az új készlet permutációját használja. A paraméter-layout minden switch készletben azonos, ezért a beállított értékek és textúrák megmaradnak |

Nézet és fények: a `RenderPassContext::SetView(ViewParameters)` beállítja a view transformot és a `u_viewParams`-t (kamera pozíció, idő, a pass targetjének mérete); a `SetLights(LightEnvironment)` a `u_lightParams`-t (egy opcionális `DirectionalLight` — az iránya a fény haladási iránya, az engine normalizálja —, legfeljebb `LightEnvironment::kMaxPointLights` = 4 `PointLight`, ég- és talaj-ambiens; a radiancia szín × intenzitás). Több pontfény, nulla irányvektor vagy nem pozitív sugár `std::invalid_argument`. Az adatokat a graph birtokolta `ViewUniformSet` tartja, minden pass elején nullázza (a passnak magának kell hívnia a `SetView` / `SetLights`-ot), és minden `Draw` előtt beállítja a két uniformot; a bgfx név szerint osztja meg őket a programokkal.

Színkezelés: a `TextureDescriptor::bSrgb` (`BGFX_TEXTURE_SRGB`) sRGB-ként mintavételezett textúrát ad (albedó), a jelenet lineáris HDR színe `Rgba16F` targetbe kerül, a `PostProcess.slang` ACES (fitted) tonemapet és `pow(1/2.2)` kódolást végez a színezés és a vignetta előtt.

Rajzolás: a `DrawCommand::pMaterial` (egy `MaterialInstance`) az anyagos út; a nyers `pProgram` / `pParameters` út megmarad teljes képernyős és post-process munkára, a kettő közül pontosan egyet kell megadni (`std::invalid_argument`). A `RenderPassContext::Draw` a vertex buffer layoutjából választ vertex factoryt (`VertexBuffer::GetLayout`; `Tangent` jelenlétében `StaticMeshTangent`, különben `StaticMesh`; hiányzó attribútum vagy eltérő komponensszám esetén `std::logic_error` az attribútumok nevével), a passból (`RenderPassBuilder::SetMaterialPass`, alapértelmezés `Forward`) a pass permutációt, lefordítja vagy előveszi a permutációt, és a feloldott programmal és paraméterekkel adja tovább a submitternek. Az első rajzolás fordít (tipikusan néhány tíz ms); előre melegíteni a `Material::CompilePermutation`-nel lehet. A Sandbox két passsal rajzolja a jelenetet ugyanabba a targetbe: a `DepthPrePass` (`SetMaterialPass(MaterialPass::DepthOnly)`, csak mélység és stencil törlés, `bColorWrite = false`) írja a mélységet, a `Scene` pass (`Forward`) csak a színt törli, és `DepthTest::LessEqual`, `bDepthWrite = false` állapottal árnyal. A kocka gráfjában a `UseNormalMap` static switch választ a normal map és a lapos `(0, 0, 1)` normál között; a talaj példánya `SetStaticSwitch("UseNormalMap", false)`-t hív, így négy permutáció fordul: `[DepthOnly, StaticMeshTangent, set0]`, `[DepthOnly, StaticMesh, set1]`, `[Forward, StaticMeshTangent, set0]`, `[Forward, StaticMesh, set1]`.

### Anyaggráf (MaterialGraph) és MaterialCompiler

Egy anyag kódból épített gráf is lehet (`MaterialGraph`), amelyből a `MaterialCompiler` ugyanolyan `MaterialDescriptor`-t állít elő, mint amilyet egy kézzel írt anyaghoz adunk; a `Material(MaterialSystem&, MaterialGraph)` konstruktor ezt fordítja le, megtartja a gráfot (`Material::GetGraph()`, leíróból készült anyagnál `nullptr`), és a generált Slang modult debug szinten logolja.

- **Csomópontok:** `Constant`, `ScalarParameter`, `VectorParameter`, `TextureParameter`, `TextureSample` (UV nélkül `TEXCOORD0`), `TexCoord` (csak 0), `VertexColor`, `VertexNormal`, `Time`, `Add` / `Subtract` / `Multiply` / `Divide`, `Lerp`, `Dot`, `Saturate`, `OneMinus`, `Power`, `Normalize`, `Abs`, `Sin`, `Cos`, `ComponentMask`, `Append`, `Fresnel` (bekötetlen kitevő = alapérték), `StaticSwitch` (név, `true` és `false` ág, alapérték; `AddStaticSwitch(name, whenTrue, whenFalse, default)`), `CustomCode` (egyetlen kifejezés `$0..$2` helyőrzőkkel). Kimenetek: `BaseColor`, `Normal` (tangens-térben), `Metallic`, `Roughness`, `AmbientOcclusion`, `Emissive`, `Opacity`, `WorldPositionOffset`.
- **Builder:** az `Add*` metódusok azonnal ellenőrzik az id-kat (`std::out_of_range`) és a textúra-bemenet típusát (`std::invalid_argument`). A gráf neve, a paraméternevek és a `CustomCode` kifejezés érvényes UTF-8 kell legyen (`std::invalid_argument`); a generált modulnévbe csak ASCII betű, számjegy és `_` kerül, minden más `_` lesz.
- **JSON:** `ToJson()` (belső `JsonWriter`) és `FromJson()` (simdjson DOM, a `Flawy` target `PRIVATE` függősége). Egy objektum: `version`, `name`, `shadingModel`, `nodes` (az id a tömbbeli pozíció; `inputs` csak a használt aritással, `-1` = bekötetlen; `value` mindig 4 szám), `outputs` (slotnév → id); a `StaticSwitch` mezői `name`, `default` (bool) és `inputs` (`[true ág, false ág]`). Ismeretlen mező, típus, verzió, ismétlődő kulcs (bármely objektumban) vagy hibás id `std::invalid_argument`-et ad JSON-útvonallal (pl. `nodes[3].mask`). `FromJson(ToJson(g)).ToJson() == g.ToJson()`.
- **Static switch-ek:** a `MaterialCompiler::Compile(graph, switchValues)` a switch-eket kódgeneráláskor oldja fel: minden `StaticSwitch` a kiválasztott ágára köt át (a meg nem adott switch az alapértékét kapja; ismeretlen név `std::invalid_argument`), így az elérhetőség és a Slang generálás csak a kiválasztott ágat látja — a nem választott ág nem generálódik. A típusellenőrzés viszont a feloldás előtt minden ágra lefut (layout-elérhetőség szerint): egy switch két ágának azonos típusúnak kell lennie (vagy az egyik `Float1`, ami szétterül), és bármely ágban lévő típushiba már az alapértelmezett készlet fordításakor, azaz a `Material` létrehozásakor `MaterialCompileError` (az eltérő ágtípus hibája a `StaticSwitch` csomópontot nevezi meg), nem csak egy későbbi `SetStaticSwitch`-nél. A paraméter-layout viszont az összes ágból elérhető paraméterekből készül, ezért minden switch készletnek ugyanaz a layoutja (slotok, stage-ek), és a `MaterialInstance` értékei készlettől függetlenek. A switch-nevek külön névteret alkotnak, egyedinek és nem üresnek kell lenniük. Az eredmény `vStaticSwitches` mezője a feloldott teljes készlet név szerint rendezve.
- **Fordítási lépések:** validálás (az összes csomóponton: bemenetek, textúrabemenetek, ismétlődő paraméter- és switch-nevek, véges értékek, `CustomCode` szabályok, ciklusfelismerés — ezért a nem elért ciklus is hiba) → layout-elérhetőség (minden switch mindkét ágával) → típusfeloldás a layout-elérhető csomópontokon (a switch-ágak típusegyezésével) → static switch feloldás → elérhetőség a bekötött kimenetekből (a nem elért csomópontok és paramétereik kimaradnak, és nincsenek típusellenőrizve, így egy holt, rossz típusú csomópont nem hiba; a `WorldPositionOffset` ágban `TextureSample`, `TexCoord`, `VertexColor`, `Time`, `Fresnel` hiba, a `VertexNormal` a vertex normál) → típusfeloldás (`Float1 ⊕ FloatN → FloatN`, más eltérés hiba; a `Normalize` legalább `Float2`-t kér; kimenetnél `Float1` szétterül, a szélesebb csonkolódik, a keskenyebb hiba) → konstans-hajtogatás (a `Power` nem hajtódik negatív alapra, illetve 0 alapra nem pozitív kitevővel, mert a GPU `pow` ott nem definiált; nem véges eredmény sem hajtódik) → közös részkifejezések összevonása → paramétercsomagolás (a layout-elérhető paraméterek, csomópont-sorrendben: `Float3`/`Float4` új slot, `Float2` az első szabad igazított pár, `Float1` az első szabad komponens; textúrák stage-e csomópont-sorrendben) → Slang generálás. A hibák `MaterialCompileError`-ok, a csomópont id-jával (`GetNode()`).
- **`CustomCode` szabályai:** egyetlen, nem üres kifejezés; tilos benne `;`, `{`, `}`, `#`, `//`, `/*`, `++`, `--`, sortörés, értékadás (`=`, a `<<=` / `>>=` is, kivéve `==`, `!=`, `<=`, `>=`), az `oOutput` azonosító és a zárójeleken (`()` / `[]`) kívüli vessző; a zárójeleknek párosaknak kell lenniük, és minden `$k` létező bemenetre mutat. A felületi függvényben az `oInput` (`SurfaceInput`) érhető el; ha a csomópont a `WorldPositionOffset` kimenetet táplálja, ott csak a `vecWorldPosition` és a `vecWorldNormal` van hatókörben, az `oInput` használata hiba. A hibák a csomóponthoz rendelt `MaterialCompileError`-ok.
- **Mélység és méret:** minden fordítási lépés egyetlen, explicit veremmel számolt topologikus (post-order) sorrenden halad, így a gráf mélysége nem okoz rekurziót (20 000 elem mély lánc Debugban is lefordul). Egy gráf legfeljebb 65 536 csomópontot tartalmazhat, különben `MaterialCompileError`.
- **sRGB jelző:** a `TextureParameter` `bSrgb` jelzője a layoutba kerül (`MaterialTextureParameter::bSrgb`; kézzel írt layoutnál alapból `false`). A `MaterialInstance::SetTexture` figyelmeztetést logol (nem dob), ha a kötött `Texture::IsSrgb()` eltér tőle; a mintavételezés módját maga a textúra határozza meg.
- **Generált modul:** neve a gráf nevéből képzett azonosító + `_` + a JSON és a feloldott switch értékek 16 hexa jegyű FNV-1a hash-e (`Core::Fnv1aHasher`; két különböző gráf vagy switch készlet nem ütközik a sessionben, az explicit alapérték és a meg nem adott switch ugyanazt a nevet adja), virtuális útvonala `Generated/<Név>.slang`. Megjegyzést nem tartalmaz. A `kUsesTangentSpaceNormal` akkor `true`, ha a `Normal` kimenet be van kötve.

### Shader cache

A `ShaderCache` a `Material::CompilePermutation` által kért specializált stage-eket tárolja; a nyers `ShaderCompiler::Compile` útvonal nem cache-el.

- **Kulcs:** `ShaderCacheKey::FromRequest(request, compiler.GetCacheFingerprint())` az engine-ben, bármilyen Slang front-end munka előtt számolódik: a cache formátumverziója, a compiler ujjlenyomata, a stage, az entry point, a pass modul neve + útvonala + forrása, minden további modul neve + útvonala + forrása és a típusargumentumok. Az ujjlenyomatot a `ShaderCompiler` egyszer, a létrehozásakor állítja össze: `kCompilerPipelineVersion`, `SLANG_TAG_VERSION`, a betöltött Slang DLL build tagje (`IGlobalSession::getBuildTagString()`), a bgfx binárisformátum verziója (12), a backend, a profil, stage-enként a target formátum, a profil id és az összes target opció (mátrix-layout, optimalizálás, debug info, az emitter `AppendTargetOptions`-a, pl. `VulkanBindGlobals`) valamint a binding szabályok, a session mátrix-layoutja, és a teljes beágyazott shadertábla (útvonal + tartalom; így az importált `Flawy/*` könyvtármodulok változása is új kulcsot ad). **A `kCompilerPipelineVersion`-t (`ShaderCompiler.cpp`) növelni kell minden olyan változtatásnál, amely a kimenetet a fenti bemenetek változása nélkül módosítja**: emitter (`IShaderBackendEmitter` implementációk), `BgfxShaderInterfaceBuilder`, `BgfxShaderBinaryWriter`, target opciók vagy a kibocsátó lépés kódja. A bemenetek hossz-prefixszel kerülnek a `Core::Fnv1aHasher`-be, két seeddel (`kDefaultSeed`, `kAlternateSeed`); a két 64 bites érték a 128 bites kulcs, `ToHex()` 32 hexa jegy.
- **Keresés:** csak lemezen: `<dir>/<hex>.flsc`. Memóriabeli másolatot a cache nem tart (a lefordított permutációkat a `Material` őrzi); könyvtár nélkül a cache ki van kapcsolva (`IsEnabled() == false`, a `Find` mindig üres, a `Store` no-op, a statisztika nem változik). Találatkor a Slang egyáltalán nem fut, és a kapott `CompiledShader` bájtra azonos a friss fordítással (bináris, név, entry point, uniformok, attribútumok, uniform buffer méret; a probe ellenőrzi). Statisztika: `GetStatistics()` (`uDiskHits`, `uMisses`, `uStores`).
- **Fájlformátum** (little endian): `"FLSC"`, u32 formátumverzió, a kulcs (2 × u64), u8 stage, név és entry point (u32 hossz + bájtok), uniformok (u32 darab × {név, u8 típus, u16 darab, u8 stage, u8 predefined}), attribútumok (u32 darab × u8), u32 uniform buffer méret, u32 binárisméret + bájtok, végül a megelőző bájtok u64 FNV-1a ellenőrzőösszege. Bármilyen eltérés (csonka fájl, rossz ellenőrzőösszeg, magic, verzió, idegen kulcs, érvénytelen enum, maradék bájt) hiány: a fájl törlődik, `warn` log, a shader újrafordul és újra tárolódik.
- **Írás:** a könyvtár az első `Store`-kor jön létre; az írás egy egyedi ideiglenes fájlba (`<hex>.flsc.<nonce>.<sorszám>.tmp`; a nonce cache-példányonként `std::random_device`-ból és az óra értékéből képzett 64 bites érték, így párhuzamos folyamatok nem írják egymás ideiglenes fájlját), majd átnevezéssel történik. I/O hiba `warn`, a fordítás eredménye ettől még használható.
- A Sandbox `--shader-cache=<dir>` opcióval kapcsolja be; leállításkor logolja a statisztikát.

Élettartam: a `MaterialInstance` a `Material`-ra, a `Material` a `MaterialSystem`-re hivatkozik. Az anyagokat és példányokat a host az `OnShutdown()`-ban szabadítja fel, előbb a példányokat, aztán az anyagokat.

## Fázisok

1. Slang integráció, `ShaderCompiler`, `BgfxShaderBinaryWriter`, Vulkan és D3D11 backend, egy default shader, és egy háromszög a Sandboxban.
2. A `ShaderLibrary` és a custom shaderek fájlból (a `ShaderCache` a 3d anyagrendszer-mérföldkőben elkészült).
3. D3D12, Metal, OpenGL/GLES.
4. WebGPU, offline bake mód, hot reload.

## Döntések

- **Slang:** az install script a GitHub release-ből tölti le a platformhoz illő előre fordított csomagot (v2026.19).
- **DXC:** szintén előre fordított GitHub release-ből jön (v1.9.2609), Windowson és Linuxon.
