# Ballistic Helmet Asset Intake

## Source received

The user supplied the same ballistic helmet model in multiple interchange/source formats plus a texture archive.

RAR member tables and archive SHA-256 values were recorded on 2026-09-07. The archive hashes identify the exact source set without committing licensed binaries to Git.

| Archive | Contents | Uncompressed size | Archive SHA-256 |
|---|---|---:|---|
| `Example_FBX.rar` | `Example.fbx` | 1,607,212 bytes | `3f6fddba75922dee40ef874779e69001622c80b37fde7657b8b194a8f2e5525b` |
| `Example_OBJ.rar` | `Example.obj`, `Example.mtl` | 4,319,997 bytes + 766 bytes | `1b5bbb38f3f71d5cf73ea472d7bea68d756e0fc96eb12d752f64c2ef2c87093d` |
| `Example_BLEND.rar` | `Example.blend` | 262,353,248 bytes | `4e2903cd78ee2d0664cb07fe84327f47def14c442fe8b67964efac4d46e15833` |
| `Example_ABC.rar` | `Example.abc` | 3,584,767 bytes | `afef5ebbccabd237d7bf17607b27ec84a7125569f3dac10c5378012c42c3fc35` |
| `Example_STL.rar` | `Example.stl` | 2,333,684 bytes | `3d4d835e27e0b9fbba43cf3f9bf0773b16c92c3b6e3868ce12886d44ef8568bf` |
| `Texture.rar` | `3_helmet_color.tga`, `3_helmet_exp.tga`, `3_helmet_norm.tga` | 4,194,854 bytes each | `edd0dc73b98070b1ae79988cc3bad2e677ad6efc7f23b8b25e8291f798a01a34` |

These are user-supplied source binaries and should remain outside the public Git repository until provenance/license/redistribution terms are recorded.

## Local intake tooling

`Tools/AssetPrep/prepare_ballistic_helmet.ps1` accepts the directory containing the six supplied RAR archives and:

- validates all six archive names
- extracts them with 7-Zip when available, with `tar`/libarchive as a fallback
- verifies every expected member exists exactly once
- stages the source under ignored `ExternalAssets/BallisticHelmet/`
- copies the preferred FBX, Blender source and three textures into a `Working` directory
- records extracted-file SHA-256 hashes and roles in `helmet_manifest.json`
- refuses to overwrite a non-empty working intake unless `-Force` is explicitly supplied

Example from the repository root:

```powershell
.\Tools\AssetPrep\prepare_ballistic_helmet.ps1 -SourceDirectory "C:\path\to\helmet-rars"
```

`Tools/Blender/ballistic_helmet_inspect.py` is a separate read-only Blender inspection pass. Run it with Blender auto-execution disabled:

```powershell
blender --background --disable-autoexec "ExternalAssets\BallisticHelmet\Working\Example.blend" --python "Tools\Blender\ballistic_helmet_inspect.py"
```

It reports mesh-object counts, vertices/polygons, disconnected geometry islands, materials, UV layers, transforms, bounds and loaded image metadata without saving or modifying the source `.blend`.

## Recommended source-of-truth hierarchy

1. **Blender file** — editable source for pivot/orientation/material inspection and any cleanup.
2. **FBX** — preferred Unreal static-mesh import path for the runtime helmet.
3. **OBJ/MTL** — geometry/material fallback/reference.
4. **Alembic** — not needed for an ordinary rigid helmet unless the source contains a specific reason to preserve Alembic data.
5. **STL** — geometry/reference fallback only; not preferred for game materials/UV workflow.

Do not import all formats into production content. They represent alternate versions of the same item.

## Unreal target

Suggested content layout:

```text
/Game/Characters/Operators/Gear/Helmets/BallisticHelmet/
  Meshes/
    SM_BallisticHelmet
  Materials/
    M_BallisticHelmet
    MI_BallisticHelmet
  Textures/
    T_BallisticHelmet_Color
    T_BallisticHelmet_Normal
    T_BallisticHelmet_Exp
  Data/
    EQ_BallisticHelmet
```

The helmet should be a **rigid static mesh** attached to the operator head/headwear socket, not a skeletal/leader-pose item unless later inspection proves the model contains deformation that genuinely requires skinning.

## Equipment definition

Create `EQ_BallisticHelmet` as `UTUEquipmentDefinition` with:

- `ItemId`: `helmet_ballistic_01`
- `DisplayName`: `Ballistic Helmet`
- `Slot`: `Headwear`
- `ThirdPersonStaticMesh`: `SM_BallisticHelmet`
- `AttachSocket`: the validated operator helmet/head socket
- `bHideThirdPersonFromOwner`: true
- no first-person mesh by default
- `bProvidesBallisticProtection`: true
- `ProtectedRegions`: `Head`

Do **not** invent protection values from the filename/model appearance. Leave `PenetrationResistance`, `MaxArmorDurability`, and stopped-round/coverage values unconfigured until the represented helmet specification or intended gameplay class is chosen.

The runtime data model uses the same numeric penetration domain as `FAmmoDefinition::Penetration`, but that is a game-model scale rather than a claim that the mesh has any particular NIJ certification.

## Texture import

### `3_helmet_color.tga`

Likely base-color/albedo candidate based on filename.

- import as color texture
- sRGB on unless visual inspection demonstrates this is not albedo
- connect to Base Color after validating the material

### `3_helmet_norm.tga`

Normal-map candidate.

- import with normal-map compression
- sRGB off
- confirm tangent-space orientation visually in Unreal

### `3_helmet_exp.tga`

The meaning of `exp` is **not established by the archive filename alone**. It may represent a specular exponent/gloss-style map or another packed authoring convention.

Do not wire it blindly into Roughness/Specular/Metallic. Inspect RGB/alpha channels in Blender/image tooling first and document the interpretation before building the final Unreal material.

## Mesh acceptance

Before the helmet is considered production-ready:

1. Run the archive intake script and retain the generated manifest.
2. Run the read-only Blender inspector and review object/island/material/UV evidence.
3. Confirm the helmet contains no unrelated scene geometry.
4. Confirm real-world-ish scale against the operator head.
5. Apply/validate transforms in a working copy, not the only original source.
6. Place the pivot so socket adjustment is stable and understandable.
7. Preserve UVs and verify the three provided texture maps line up.
8. Import the FBX into Unreal as `SM_BallisticHelmet`.
9. Use simple collision only where useful for physical interaction; do not rely on render-triangle collision as the permanent ballistic damage resolver.
10. Attach to the operator headwear socket and tune `RelativeTransform` in `EQ_BallisticHelmet`.
11. Test idle, sprint, crouch, lean, ADS, prone/low-ready when those poses exist.
12. Verify the local first-person camera never renders/clips through the third-person helmet.
13. Verify remote players see the helmet and its shadow correctly.
14. Test compatibility with headset, eyewear, facewear and NVG slots.

## Ballistic runtime behavior now implemented

PR #18 now contains `UTUArmorProtectionComponent` and the modular operator owns it alongside regional health.

The runtime path is:

```text
incoming ballistic hit
  -> equipped protection lookup by body region and coverage
  -> penetration vs authored resistance
  -> per-operator armor durability damage
  -> stopped-round residual damage OR penetrating raw damage
  -> UTUHealthComponent::ApplyRegionalDamage(region, remaining damage)
```

Important properties:

- durability is per operator instance, not stored on the shared `UTUEquipmentDefinition`
- two operators can wear the same helmet definition and damage their armor independently
- replacing a helmet resets runtime durability to the new item's authored maximum
- 0% coverage never protects
- partial coverage uses a deterministic `CoverageRoll01` input until precise armor hit geometry exists
- a stopped projectile can still apply authored residual/blunt damage
- penetrating projectiles can still consume armor durability
- armor with zero/unconfigured resistance or durability does not create fake protection

Automation coverage:

- `TheUnit.Operator.Armor.BallisticProtection`
- `TheUnit.Operator.Equipment.State`
- `TheUnit.Operator.Equipment.GameModeWiring`

Precise helmet collision coverage, ricochet/deflection, impact angle, material-specific penetration, multiplayer replication of armor state and visual damage remain future combat-layer work.

## Provenance gate

No author, marketplace/source URL, license, purchase record, or stated real-world helmet rating was included with the archive names inspected here. Record those details before redistribution/shipping and before assigning a real-world ballistic certification label to the in-game item.
