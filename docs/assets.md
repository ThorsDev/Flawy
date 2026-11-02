# Flawy – Asset rendszer

> Állapot: az 1. fázis kész (`Flawy::Assets` alap, `AssetManager`, loader/importer regisztráció, közös `.fl*` bináris fejléc, `StaticMeshAsset` és `.flmesh`). A 2. fázis is kész: az animációs assetek (`.flskel`, `.flclip`, `.flctrl`) leírása az [animation.md](animation.md)-ben van. A 3. fázis is kész: a `Flawy::Import` modul glTF, FBX és OBJ importerei (lásd [Import](#import-gltf-fbx-obj)).

## Fogalmak

| Típus | Felelősség |
|---|---|
| `Asset` | minden asset absztrakt őse: `AssetId`, név, `GetTypeId()`; nem másolható, nem mozgatható |
| `AssetId` | normalizált, az asset rootra relatív út + annak 64 bites FNV-1a hash-e. Ez az egyetlen hely, ahol később GUID-ra lehet váltani |
| `AssetTypeId` | egy típusnév constexpr FNV-1a hash-e (`AssetTypeId::FromName("StaticMesh")`). Minden asset osztály `static constexpr AssetTypeId kTypeId`-t ad |
| `AssetHandle<T>` | `std::shared_ptr<T>` burkoló; üres handle-n a `Get`, `operator->`, `GetId` `std::logic_error`-t dob |
| `IAssetLoader` | egy engine-formátum (kiterjesztés) betöltője; nyers bájtokat és `AssetLoadContext`-et kap |
| `IAssetImporter` | forrásformátum (pl. `.gltf`, `.fbx`, `.obj`) importere; `AssetImportContext`-et kap, `AssetImportResult`-ot ad |
| `AssetManager` | cache, regisztrációk, útvonal-feloldás, betöltés, import |

Az `AssetId` és az `AssetTypeId` hash-e a `Core::Fnv1aHasher::HashText`-tel készül: ez a szöveg bájtjainak nyers FNV-1a hash-e. A `Fnv1aHasher::Append(std::string_view)` ezzel szemben a szöveg elé a hosszát is bekeveri (u64), hogy több egymás után fűzött szöveg ne ütközhessen. A kettő ezért ugyanarra a szövegre különböző értéket ad; az azonosítókhoz mindig a `HashText`-et kell használni, mert az a fájlokba írt típusazonosítók alapja.

## Útvonalak

- Minden asset egyetlen asset root alá esik (`ApplicationSpecification::optAssetRoot`, a konstruktor abszolúttá teszi). Asset root nélkül minden `Load` / `ResolvePath` `std::logic_error`-t dob.
- `AssetManager::NormalizePath`: `\` → `/`, a kezdő `./` levágása. Hibás (`std::invalid_argument`): üres út, abszolút út (`/...`, `C:...`), üres, `.` vagy `..` szegmens.
- A loader keresésekor a kiterjesztés kisbetűsítve számít; maga az út nem változik. A regisztrált kiterjesztéseknek kisbetűsnek kell lenniük és `.`-tal kell kezdődniük.
- Az asset utak kis- és nagybetű-érzékenyek, és pontosan a lemezen lévő írásmódot kell követniük. Windowson a fájlrendszer ugyan megtalálja a `generated/cube.flmesh`-t is, de az a `generated/Cube.flmesh`-től különböző `AssetId`, így ugyanaz a fájl két külön cache-bejegyzésként töltődne be.

## AssetManager

- `RegisterLoader` / `RegisterImporter`: a composition root (`Core::Application` az engine típusokhoz, a host a sajátjaihoz) explicit regisztrál; nincs globális regisztráció. Ugyanarra a kiterjesztésre második regisztráció `std::invalid_argument`.
- `Load<T>(path)` / `LoadUntyped(path[, type])`: normalizálás → cache (találat esetén típusellenőrzés, eltérés `std::logic_error`) → loader a kiterjesztés alapján (ha nincs: `std::invalid_argument`) → `ResolvePath` → `Core::FileBytes::Read` (hiányzó fájl: `std::runtime_error` az abszolút úttal) → `IAssetLoader::Load` → a visszaadott asset típusa és `AssetId`-ja ellenőrizve (`std::logic_error`) → cache. Info log: `Loaded asset '<path>' (<type>) in <n> ms`. Hiba esetén semmi nem kerül a cache-be.
- Egy loader betölthet más asseteket (`oContext.oManager`); a körkörös függőséget a manager felismeri (`std::logic_error("Asset dependency cycle: a -> b -> a")`).
- `Insert<T>(path, asset)`: kódban létrehozott assetet regisztrál; az asset `AssetId`-jának egyeznie kell a normalizált úttal, a `GetTypeId()`-jének a `T::kTypeId`-vel, foglalt útra `std::invalid_argument`.
- `Import(sourcePath, options)`: normalizálja az utat, a forrás kiterjesztéséhez regisztrált importert keresi (ha nincs: `std::invalid_argument` a kiterjesztéssel), ellenőrzi, hogy a forrásfájl létezik (különben `std::runtime_error`), majd meghívja az importert. Minden létrejött assetre info logot ír: `Imported asset '<path>' (<type>) from '<source>'`.
- `Unload(path)` mindenképp kiveszi a bejegyzést; `UnloadUnused()` addig ismétli a `use_count() == 1` bejegyzések törlését, amíg talál ilyet (így a más assetek által tartott függőségek is felszabadulnak).
- A destruktor először ugyanazt a fixpontos felszabadítást futtatja, mint az `UnloadUnused()` (így a más assetek által tartott függőségek, pl. egy klip csontváza, nem adnak hamis riasztást), majd warn logot ír minden olyan megmaradt assetre, amelyre még külső handle mutat.
- Szinkron betöltés; a `IAssetLoader::Load` bájtokat és kontextust kap, így az aszinkron olvasás később a manageren belül bevezethető.
- Az `AssetManager` nem szálbiztos: csak a fő szálról használható. A loaderek a hívó szálon futnak, és GPU-erőforrást is létrehozhatnak (pl. a `StaticMeshAsset` vertex- és indexbufferei), ami a bgfx egyszálú módja miatt szintén a fő szálat igényli.

## Élettartam

Az `Application` birtokolja az `AssetManager`-t: a `ShaderLibrary` után, a `RenderGraph` előtt jön létre, és közvetlenül az `OnShutdown()` után, a `RenderGraph` előtt szűnik meg, így a GPU-erőforrást tartó assetek (pl. `StaticMeshAsset` bufferei) még élő bgfx eszközön szabadulnak fel. A host a GPU-alapú assetekre mutató handle-jeit az `OnShutdown()`-ban engedje el.

## Bináris formátum (`.fl*`)

Minden bináris engine-formátum közös burkot használ (`AssetFile`), little-endian, explicit bájt-shiftekkel:

| Offset | Méret | Mező |
|---|---|---|
| 0 | 4 | magic `FLAW` |
| 4 | 4 | u32 fejléc verzió (1) |
| 8 | 8 | u64 `AssetTypeId` |
| 16 | 4 | u32 formátum verzió (típusonként) |
| 20 | 8 | u64 payload méret |
| 28 | n | payload |
| 28+n | 8 | u64 FNV-1a az összes előző bájtra |

A `AssetFile::Decode` `Core::BinaryFormatError`-t dob (bájt offsettel) rossz magic, fejléc verzió, típuseltérés (mindkét id-vel), méreteltérés vagy checksum-hiba esetén. A `Core::BinaryReader` minden olvasást határellenőriz, a hosszakat a hívó által megadott maximumhoz méri, a nem véges floatokat és a 0/1-től eltérő boolt elutasítja, és allokálás előtt ellenőrzi, hogy a kért elemszám elfér-e a maradék bájtokban. Fájlba írás: `Core::FileBytes::WriteAtomically` (ideiglenes fájl + rename).

## `.flmesh` (StaticMesh, formátum verzió 1)

Payload:

1. név (u32 hossz + bájtok, max. 4096)
2. u32 vertexszám (1 .. 16M)
3. u8 flagek: bit0 normál, bit1 tangens, bit2 uv0, bit3 szín, bit4 skinning
4. pozíciók (3 × f32 vertexenként), majd a flagek szerint jelen lévő streamek ugyanebben a sorrendben: normál (3 × f32), tangens (4 × f32), uv0 (2 × f32), szín (u32 ABGR), joint indexek (4 × u8), joint súlyok (4 × f32)
5. indexek (u32 darabszám + u32 elemek, max. 16M)
6. u32 section-szám (max. 65536), sectionönként: anyagnév (string), u32 első index, u32 indexszám

Dekódoláskor a `MeshData::Validate` is lefut (a hibája `BinaryFormatError`-ként jelenik meg).

`MeshData::Validate` (`std::invalid_argument`): van pozíció; a normál/tangens/uv/szín/joint streamek üresek vagy vertexszám méretűek; joint indexek és súlyok csak együtt; minden float véges, a súlyok nemnegatívak; az indexek száma nem nulla, 3 többszöröse, mind kisebb a vertexszámnál; a sectionök sorrendben, átfedés és rés nélkül fedik le az indexeket (section nélkül egy implicit `Default` section jön létre).

`StaticMeshAsset`: egy `VertexBuffer` (a meglévő `StaticMesh` vagy tangens esetén `StaticMeshTangent` vertex layout; hiányzó normál = (0,1,0), uv = 0, szín = 0xffffffff) és sectionönként egy `IndexBuffer` (u16, ha a vertexszám ≤ 65535, különben u32). A skinning streamek a CPU oldali `MeshData`-ban maradnak; a GPU skinning későbbi lépés.

## Import (glTF, FBX, OBJ)

A `Flawy::Import` modul (`Flawy/include/Flawy/Import/`, `Flawy/src/Import/`) három `IAssetImporter`-t ad, amelyeket a `Core::Application` regisztrál: glTF (`.gltf`, `.glb`, fastgltf), FBX (`.fbx`, ufbx) és OBJ (`.obj`, fast_obj). Mindhárom egy közös `ModelImporter` példány, amely egy privát `IModelReader`-t (`GltfModelReader`, `FbxModelReader`, `ObjModelReader`) használ; a harmadik féltől származó fejlécek csak ezek `.cpp` fájljaiban jelennek meg. A host a `ModelImporterFactory`-val saját `AssetManager`-hez is létrehozhatja őket.

Folyamat: az olvasó egy CPU oldali `ImportedScene`-t ad (`MeshData`-k, opcionális `SkeletonDescriptor`, `AnimationClipDescriptor`-ok, figyelmeztetések), a `ModelImporter` warn szinten logolja a figyelmeztetéseket, majd az összes assetet létrehozza és validálja (`StaticMeshAsset`, `SkeletonAsset`, `AnimationClipAsset`), és csak ezután ment és regisztrál. Összegző log: `Imported '<source>': <n> mesh(es), <n> skeleton(s), <n> clip(s)[; saved <n> asset(s) to '<dir>']`.

`AssetImportOptions`:

- `bImportMeshes`, `bImportSkeletons`, `bImportAnimations`: szűrők. Csontváz nélkül klip nem importálható (warn), és a mesh-ek skinning adatai elvesznek.
- `bSaveToAssetRoot = true`: a kimenet a `<sOutputDirectory vagy a forrás könyvtára>/<forrás neve>/<Név>.flmesh|.flskel|.flclip` alá kerül, a klipek `sSkeletonPath`-ja a mentett `.flskel`. Az assetek ezeken az utakon regisztrálódnak (`AssetManager::Insert`), és később a loaderekkel újra betölthetők. Az `AssetImportResult::vSavedPaths` a mentett utakat sorolja fel.
- `bSaveToAssetRoot = false`: az assetek virtuális úton regisztrálódnak: `<forrás>#<Név>` (pl. `models/Cube.obj#Cube`). Ezek nem tölthetők be lemezről, a klipek `sSkeletonPath`-ja üres.
- Ismételt importnál a már regisztrált azonos utú asset kikerül a cache-ből, és az új lép a helyére (a régi handle-ök a régi objektumot tartják).

Szabályok:

- Egységek és tengelyek: méter, jobbkezes, Y-felfelé. A glTF eleve ilyen; az FBX-et az ufbx alakítja át (`target_axes = right_handed_y_up`, `target_unit_meters = 1`, `UFBX_SPACE_CONVERSION_TRANSFORM_ROOT`); az OBJ-t ilyennek tekintjük. Az OBJ és FBX uv-k v koordinátája tükröződik (`1 - v`), hogy a glTF-fel azonos, bal-felső origójú konvenciót kövessenek.
- Mesh-ek: glTF-ben node-onként, FBX-ben mesh-példányonként egy `StaticMeshAsset`, OBJ-ből egy. Primitívenként (glTF), material partonként (FBX) és anyagonként (OBJ) egy section, amelynek `sMaterialName`-je a forrás anyagnevét viseli. Textúrák és anyagok nem importálódnak. Az FBX mesh-ek sarokpontonként épülnek, majd hegesztődnek: a minden streamjükben (pozíció, normál, tangens, uv, szín, joint index, súly) bitre azonos vertexek összevonódnak.
- Statikus mesh-ekbe a node világtranszformációja be van égetve (glTF-ben csak a skin nélküli node-oknál: a skinnelt node vertexei a glTF szerint bind térben vannak, ezért akkor sincs beégetés, ha a skin nem importálódik); negatív determinánsnál a háromszögek körüljárása megfordul, és a tangensek `w` előjele is.
- Hiányzó normálok: glTF primitívenként és OBJ-ben a háromszögekből számolva (súlyozott átlag; OBJ-ben csak a normál nélküli sarkokból lett vertexekre, a megadott normálok megmaradnak), FBX-ben az ufbx generálja. A háromszögsáv és -legyező átalakul háromszöglistává; a pont- és vonalprimitívek kimaradnak (warn).
- Egy fájlból egy csontváz. glTF: az első, mesh által használt skin (ha egyik sem, a 0. skin); a többi skinre warn, az azokhoz kötött mesh-ek skinning adat nélkül, bind pózban importálódnak. FBX: az összes skin deformer, amelynek csontjai ugyanahhoz a csontváz-gyökérhez (a csontok közös ősének legfelső, a scene root alatti őse) tartoznak, egy csontvázzá egyesül, és minden ilyen mesh skinnelt lesz; az eltérő gyökerű deformerekre warn, a mesh-eik statikusak. Mesh-enként csak az első skin deformer számít.
- Csontváz: glTF-ben a skin jointjai a skin inverse bind mátrixaival (hiányzó accessor = identitás); FBX-ben az egyesített cluster csontok és a közös ősükig vezető node-ok. Mesh-enként egy `G` beégetési mátrix, és csontonként egy közös inverse bind: `IBM_j = geometry_to_bone_j * inverse(G)`. Az első mesh `G`-je a bind-kori geometria-világ mátrix (`bind_to_world * geometry_to_bone` az első clusteréből); a későbbi mesh-ek `G`-je egy már ismert csontú clusterből adódik (`inverse(IBM_j) * geometry_to_bone_j`), így a közös IBM-ek minden mesh-re helyesek. Ha egy mesh clusterei ennek ellentmondanak (eltérő bind pózok), warn és közelítés. A gyökér joint (és a nem közvetlen szülőjű joint) lokális transzformációja tartalmazza a csontvázon kívüli ősök nyugalmi transzformációját; ezek animálását nem követjük, nem uniform örökölt skálánál warn.
- A skinnelt mesh-ek joint indexei a forrás joint-sorrendből a futásidejű `Skeleton::FindJoint` sorrendre vannak átképezve; jointonként legfeljebb 4 súly (normalizálva), egy vertex csak 256 alatti futásidejű indexű jointra hivatkozhat nem nulla súllyal (különben warn és a mesh skinning adata elvész; a nem hivatkozott jointok indexe nem számít). A súly nélküli FBX vertexek az első jointhoz kötődnek.
- Animációk: csak a csontváz jointjait célzó csatornák. glTF Linear változatlan, Step duplikált kulcsokkal (a váltás előtt 0,1 ms-mal az előző érték), CubicSpline 30 Hz-en Hermite-mintavételezve (warn); morph target súlyok kihagyva (warn). FBX: `ufbx_bake_anim` stackenként (`trim_start_time`). A klip a legkorábbi kulcsnál kezdődik (0-ra tolva). A CubicSpline újramintavételezés legfeljebb `AnimationClipDescriptor::kMaxKeysPerComponent` kulcsot ad, különben a fájl hibás (`std::runtime_error`).
- Hibás glTF-ek: a fastgltf validációja előtt saját ellenőrzés fut: minden buffer betöltött mérete legalább a deklarált `byteLength`, a buffer view-k a bufferen belül vannak, a sparse accessorok view indexei érvényesek, az index típusa u8/u16/u32, az indexek és értékek a view-jukon belül vannak; olvasáskor a sparse indexeknek szigorúan növekvőnek és az accessor elemszáma alattinak kell lenniük. Hiba esetén `std::runtime_error` (`glTF file '<path>' is invalid: ...`).
- Az asset nevek importon belül egyediek (kis-nagybetű függetlenül), fájlnévként biztonságosak (tiltott karakterek `_`-ra cserélve, legfeljebb 96 bájt).
- GPU skinning továbbra sincs: a skinning streamek a `MeshData`-ban és a `.flmesh`-ben megmaradnak, a `StaticMeshAsset` nyugalmi pózban rajzolódik.

`SampleModelWriter::WriteSkinnedTubeGlb(path)`: teszt célú, a fastgltf `Exporter`-ével írt `.glb` (24 vertexes, két jointos skinnelt cső, `TubeMesh` mesh, `Tube` skin, 2 s-os `Bend` animáció).

## Shaderek és anyagok

A shaderek a `ShaderLibrary`-ben maradnak, nem assetek. Az anyagok asset-útja később egy `MaterialGraphAsset` loader (`.flmat`, a meglévő `MaterialGraph` JSON-ra építve) lesz.

## Sandbox

Az asset root a `Flawy-Sandbox/assets/` (`FLAWY_SANDBOX_ASSET_DIR`, felülírható: `--asset-root=<dir>`). A generált fájlok a `Flawy-Sandbox/assets/generated/` alá kerülnek (gitignored). Első futáskor a sandbox elmenti a kockát (`generated/Cube.flmesh`), majd kétszer betölti az `AssetManager`-rel (egy betöltés, egy cache-találat), és a jelenet kockái ebből az assetből rajzolódnak.

Import: az `ImportFactory` alapból importálja a repóban lévő `models/Cube.obj`-t (virtuális úton, x = +2 körül rajzolva), legenerálja a `generated/models/Tube.glb`-t (ha hiányzik), importálja `bSaveToAssetRoot`-tal a `generated/imported/Tube/` alá, majd a mentett `TubeMesh.flmesh` / `Tube.flskel` / `Bend.flclip` fájlokat az `AssetManager`-rel újra betölti. A csövet nyugalmi pózban, mellette a kódban létrehozott egyállapotú controllerrel lejátszott `Bend` klip jointjait kis kockákként rajzolja. `--import=<asset path>`: tetszőleges támogatott fájlt importál (mentés nélkül), a mesh-eit kirajzolja, és ha van csontváz és klip, az első klipet lejátssza.
