# Flawy – Animáció

> Állapot: a 2. fázis kész (`Flawy::Animation`: csontváz, klipek, póz-mintavételezés és -keverés, animation controller állapotgéppel, `.flskel` / `.flclip` / `.flctrl` assetek). A 3. fázisban a glTF/FBX importerek is csontvázat és klipeket állítanak elő (lásd [assets.md](assets.md#import-gltf-fbx-obj)); a GPU skinning későbbi lépés.

## Modul

- Namespace: `Flawy::Animation`, publikus headerek: `Flawy/include/Flawy/Animation/`, implementáció: `Flawy/src/Animation/`.
- Függ: Core, Assets, GLM (publikus), ozz-animation 0.17.0 (`PRIVATE`), simdjson (`PRIVATE`, a `.flctrl`-hez).
- Az ozz típusai kizárólag a `src/Animation/Ozz/*` privát headerekben és a `.cpp` fájlokban jelennek meg; a publikus osztályok pimpl-t használnak. Nincs `.ozz` fájl: az ozz runtime objektumok betöltéskor, memóriában épülnek a Flawy saját leíróiból.

## Típusok

| Típus | Felelősség |
|---|---|
| `JointTransform` | lokális TRS (`vecTranslation`, `quatRotation`, `vecScale`) |
| `SkeletonDescriptor` / `JointDescriptor` | szerkeszthető csontváz-leírás; a szülőnek korábbi indexűnek kell lennie (`kNoParent = -1` a gyökér). `optInverseBindMatrix` opcionális (importerek töltik ki), különben a modell-térbeli nyugalmi póz inverze |
| `Skeleton` | ozz `Skeleton` pimpl mögött. Jointok a runtime (mélységi bejárás) sorrendjében; `FindJoint(name)`, nyugalmi pózok (lokális és modell-térbeli), inverse bind mátrixok |
| `AnimationClipDescriptor` / `JointTrackDescriptor` | szerkeszthető klip: név, `sSkeletonPath` (relatív asset út), hossz másodpercben, jointonként transzláció/rotáció/skála kulcsok (abszolút idő másodpercben) |
| `AnimationClip` | ozz `Animation` egy adott `Skeleton`-hoz kötve. A hiányzó track vagy komponens a nyugalmi póz értékét kapja (egy kulcs t=0-ban) |
| `LocalPose` | SoA lokális póz egy csontvázhoz (`ResetToRestPose`) |
| `ModelPose` | `LocalToModelJob`; joint mátrixok és skinning mátrixok (`joint * inverseBind`) `glm::mat4`-ként |
| `AnimationSampler` | egy klip mintavételezője saját ozz `SamplingJob::Context`-tel; `SampleAtPhase` (0..1, clampelve), `SampleAtTime` |
| `PoseBlender` | `BlendingJob` előre lefoglalt rétegtömbbel; `Blend(span<BlendLayer>, LocalPose&)` |
| `AnimationControllerDescriptor` | a `.flctrl` JSON leírása (`FromJson`) |
| `AnimationController` | ellenőrzött, indexekre feloldott, megváltoztathatatlan állapotgép-leírás |
| `IAnimationClipProvider` / `AnimationClipLibrary` | klipek név szerint; a könyvtár egyetlen csontvázhoz tartozik, a klip assetek handle-jeit életben tartja |
| `Animator` | egy controller futó példánya: paraméterek, állapot, crossfade, `ModelPose` |
| `SkeletonAsset`, `AnimationClipAsset`, `AnimationControllerAsset` | asset osztályok (`.flskel`, `.flclip`, `.flctrl`), loaderjeiket a `Core::Application` regisztrálja |

Hibák (`std::invalid_argument`, a csontváz/klip/controller és az elem nevével): üres vagy 1024-nél több joint, üres vagy ismétlődő jointnév, hibás szülőindex, nem véges érték, nulla hosszú kvaternió; klipnél üres név, nem pozitív hossz, ismeretlen vagy ismétlődő joint track, nem szigorúan növekvő vagy `[0, hossz]`-on kívüli kulcsidő. Más csontvázhoz kötött klip a `AnimationClipLibrary`-be vagy egy `Animator`-ba téve szintén `std::invalid_argument`.

## Bináris formátumok

Mindkettő a közös `AssetFile` burkot használja (lásd [assets.md](assets.md)), little-endian.

### `.flskel` (Skeleton, formátum verzió 1)

1. név (string, max. 4096)
2. u32 jointszám (1 .. 1024)
3. jointonként: név (string), i32 szülőindex, 3 × f32 transzláció, 4 × f32 rotáció (x, y, z, w), 3 × f32 skála, u8 van-e inverse bind (0/1), és ha van: 16 × f32 (oszlopfolytonos)

### `.flclip` (AnimationClip, formátum verzió 1)

1. csontváz asset út (string; kódban épített klipnél üres lehet, de a loader ilyenkor hibát ad)
2. név (string)
3. f32 hossz másodpercben
4. u32 trackszám (max. 1024)
5. trackenként: jointnév (string); transzláció kulcsok (u32 darab + darabonként f32 idő + 3 × f32); rotáció kulcsok (u32 + f32 idő + 4 × f32 x,y,z,w); skála kulcsok (u32 + f32 idő + 3 × f32). Komponensenként max. 1M kulcs

Dekódoláskor a leíró `Validate()`-je is lefut; a hibája `Core::BinaryFormatError`-ként jelenik meg. Az `AnimationClipLoader` a `sSkeletonPath`-ot az `AssetManager`-rel tölti be, így a klip asset a csontváz assetet életben tartja.

## `.flctrl` (AnimationController, JSON)

```json
{
  "version": 1,
  "name": "SandboxArm",
  "parameters": [
    { "name": "Energy", "type": "float", "default": 0.0 },
    { "name": "Grounded", "type": "bool", "default": true },
    { "name": "Wave", "type": "trigger" }
  ],
  "entry": "Idle",
  "states": [
    { "name": "Idle", "blend": { "parameter": "Energy", "entries": [ { "threshold": 0.0, "clip": "Idle" }, { "threshold": 1.0, "clip": "Sway" } ] }, "speed": 1.0, "loop": true },
    { "name": "Wave", "clip": "Wave", "loop": false }
  ],
  "transitions": [
    { "from": "Idle", "to": "Wave", "duration": 0.15, "conditions": [ { "parameter": "Wave", "triggered": true } ] },
    { "from": "Wave", "to": "Idle", "duration": 0.25, "exitTime": 0.95 },
    { "from": "*", "to": "Idle", "duration": 0.2, "conditions": [ { "parameter": "Grounded", "equals": false } ] }
  ]
}
```

Szabályok (a hibaüzenet útvonallal: `Animation controller JSON states[1].clip: ...`; ismeretlen mező és ismétlődő kulcs hiba):

- Kötelező: `version` (1), `name`, `entry`, `states`. Opcionális: `parameters`, `transitions`.
- Paraméter: `name`, `type` ∈ `float` | `bool` | `trigger`; `default` floatnál szám (alapból 0), boolnál bool (alapból false), triggernél tilos.
- Állapot: `name`, pontosan egy a `clip` és a `blend` közül; `speed` (alapból 1, véges és pozitív), `loop` (alapból true). `blend`: `parameter` (float paraméter) és `entries` (≥ 1, `threshold` szigorúan növekvő, `clip`).
- Átmenet: `from` (állapot vagy `*`), `to`, `duration` (alapból 0.2, ≥ 0), `exitTime` (opcionális, 0..1), `conditions` (opcionális). Kell legalább egy feltétel vagy exit time; `*` átmenetnek nem lehet exit time-ja; `from` ≠ `to`.
- Feltétel: `parameter` és pontosan egy: `greater` / `less` (szám, float paraméter), `equals` (bool, bool paraméter), `triggered` (csak `true`, trigger paraméter).

A szemantikai hibák (`std::invalid_argument`, `Animation controller '<név>': ...`) az `AnimationController` konstruktorában jönnek; az ismeretlen klipnév az `Animator` létrehozásakor (a klipeket az `IAnimationClipProvider` adja).

## Animator szemantika

- Minden allokáció a konstruktorban történik (állapotonként és blend-bejegyzésenként egy `AnimationSampler`, öt `LocalPose`, egy `PoseBlender`, egy `ModelPose`). Az `Update` nem foglal heap memóriát (a debug log csak akkor formáz, ha a debug szint engedélyezett).
- `Update(dt)`: nem véges `dt` → `std::invalid_argument`, negatív → 0.
  1. Az aktuális (és crossfade közben a cél-) állapot fázisa nő: `phase += dt * speed / duration`. Loop esetén 1-nél körbefordul, különben 1-en megáll. Blend space hossza a két aktív klip hosszának interpolációja (fázisszinkron keverés).
  2. Crossfade közben nincs új átmenet (v1 korlát); a crossfade a hossza leteltével befejeződik.
  3. Különben először az any-state (`*`) átmenetek, majd az aktuális állapotból induló átmenetek kerülnek sorra, deklarációs sorrendben; az első teljesülő indul. Feltétel: minden condition igaz, és ha van exit time: loop állapotnál a fázis ebben a frame-ben lépte át (körbefordulással együtt), nem loop állapotnál elérte. Induláskor az átmenet trigger feltételei törlődnek; 0 hosszú átmenet azonnal vált.
  4. Blend space: a paraméter az első küszöb alatt az első, az utolsó felett az utolsó klipet adja, közte a két szomszédot keveri `(p - tA) / (tB - tA)` súllyal.
  5. Crossfade súly `min(eltelt / hossz, 1)`; a két állapot pózát a `PoseBlender` keveri, majd a `ModelPose` frissül.
- Debug log átmenet induláskor: `Animator '<controller>': <from> -> <to> (crossfade <d> s)`.
- `GetCurrentStateIndex()` crossfade alatt még a forrásállapotot adja, `GetTargetStateIndex()` a célt.
- Élettartam: az `Animator` hivatkozik a controllerre, a csontvázra és a klip providerre; ezeknek túl kell élniük.

## Korlátok

- Crossfade közben nem indul új átmenet; nincs megszakítható átmenet.
- 1D blend space; nincs additív réteg, maszk vagy IK (az ozz támogatja, később bevezethető).
- GPU skinning még nincs; a `ModelPose::GetSkinningMatrices()` már előállítja a mátrixokat, és az importált mesh-ek joint indexei/súlyai a `MeshData`-ban vannak. Az importált skinnelt mesh-ek nyugalmi pózban rajzolódnak.
- Importált csontvázaknál a skinning a forrás inverse bind mátrixait használja (`JointDescriptor::optInverseBindMatrix`; FBX-ben több skin deformer egy közös csontvázba egyesülhet, lásd assets.md); ha a forrás nyugalmi póza eltér a bind póztól (gyakori FBX-ben), a nyugalmi pózú skinnelés nem esik egybe a mentett vertexekkel, ez a forrás szemantikája.
- Importált klipeknél a csontvázon kívüli ősök animációja nem kerül át; a glTF CubicSpline 30 Hz-en újramintavételezett.
- Az `AssetManager`-hez hasonlóan az animációs objektumok sem szálbiztosak.

## Sandbox

Az `AnimationFactory` első futáskor kódból építi és elmenti a négy jointos kar csontvázát (`Root` → `Spine` → `UpperArm` → `Forearm`) és három klipet (`Idle` 2 s, `Sway` 2 s, `Wave` 1 s) a `generated/animation/` alá, majd az `AssetManager`-rel betölti őket és a repóban lévő `animation/SandboxArm.flctrl`-t. Az `AnimationDriver` az `Energy` paramétert szinuszosan mozgatja, 1 s-tól 3 s-onként `Wave` triggert ad, és info logot ír állapotváltáskor (`Animator state: Idle -> Wave at 1.00 s`). A jointokat a kocka mesh kis, accent anyagú példányai jelölik a jelenet bal oldalán. Az importált `Tube.glb` csontvázát és `Bend` klipjét egy kódban épített, egyetlen `Play` állapotú controller játssza le (`ImportFactory`), jointjai a jobb hátsó részen láthatók.
