# Ballistic Helmet Asset Intake

## Source received

The user supplied the same ballistic helmet model in multiple interchange/source formats plus a texture archive.

Archive inventory inspected on 2026-09-07:

| Archive | Contents | Uncompressed size |
|---|---|---:|
| `Example_FBX.rar` | `Example.fbx` | 1,607,212 bytes |
| `Example_OBJ.rar` | `Example.obj`, `Example.mtl` | 4,319,997 bytes + 766 bytes |
| `Example_BLEND.rar` | `Example.blend` | 262,353,248 bytes |
| `Example_ABC.rar` | `Example.abc` | 3,584,767 bytes |
| `Example_STL.rar` | `Example.stl` | 2,333,684 bytes |
| `Texture.rar` | `3_helmet_color.tga`, `3_helmet_exp.tga`, `3_helmet_norm.tga` | 4,194,854 bytes each |

These are user-supplied source binaries and should remain outside the public Git repository until provenance/license/redistribution terms are recorded.

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

Do **not** invent protection values from the filename/model appearance. Leave `PenetrationResistance`, `MaxArmorDurability`, `StoppedRoundDamageMultiplier`, and coarse coverage at conservative/unconfigured values until the real helmet specification or intended gameplay class is chosen.

The runtime data model intentionally uses the same numeric penetration domain as `FAmmoDefinition::Penetration` so a later armor resolver can compare ammunition and protective equipment without hard-coding real-world certification claims into the mesh asset.

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

1. Open the editable Blender source and confirm the helmet is the intended object with no unrelated scene geometry.
2. Confirm real-world-ish scale against the operator head.
3. Apply/validate transforms in the working copy, not the only original source.
4. Place the pivot so socket adjustment is stable and understandable.
5. Preserve UVs and verify the three provided texture maps line up.
6. Import the FBX into Unreal as `SM_BallisticHelmet`.
7. Use simple collision only where needed; do not rely on render-triangle collision as the permanent ballistic hit resolver.
8. Attach to the operator headwear socket and tune `RelativeTransform` in `EQ_BallisticHelmet`.
9. Test idle, sprint, crouch, lean, ADS, prone/low-ready when those poses exist.
10. Verify the local first-person camera never renders/clips through the third-person helmet.
11. Verify remote players see the helmet and its shadow correctly.
12. Test compatibility with headset, eyewear, facewear and NVG slots.

## Ballistic gameplay boundary

The current PR adds **protective metadata**, not a complete projectile/armor solver. The helmet can be authored as head protection now while the later combat pass handles:

- actual hit location versus helmet coverage
- penetration comparison
- armor durability loss from `ArmorDamage`
- stopped-round residual/blunt damage
- ricochet/deflection where appropriate
- helmet destruction or degraded protection
- replication of per-instance armor state

This keeps visual equipment, authored protective properties and runtime damage state separated cleanly.

## Provenance gate

No author, marketplace/source URL, license, purchase record, or stated real-world helmet rating was included with the archive names inspected here. Record those details before redistribution/shipping and before assigning a real-world ballistic certification label to the in-game item.
