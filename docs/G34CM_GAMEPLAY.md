# G34 CM Gameplay / Unreal Integration

## Purpose

This document covers the supplied G34 Combat Master-style visual reference as an external game-art target and the `ATU_G34CM` runtime weapon profile.

The generated art is game geometry only. It is not manufacturing/CAD geometry and does not model a functional internal firearm mechanism.

## Generated external source package

`G34CM_GameReady_Source.zip` contains:

- `SM_G34CM_Frame.glb`
- `SM_G34CM_Slide.glb`
- `SM_G34CM_Optic.glb`
- `SM_G34CM_Magazine.glb`
- `SM_G34CM_Magwell.glb`
- `SM_G34CM_BarrelVisual.glb`
- `G34CM_Reference_Assembled.glb`
- `G34CM_socket_manifest.json`

The split mirrors the gameplay/presentation needs rather than treating the pistol as one static prop.

## Visual target represented

- long competition/tactical pistol silhouette
- aggressive front/rear slide treatment
- black textured frame
- copper/bronze external barrel-window visual accent
- slide-mounted reflex optic
- flared magwell
- extended magazine/base
- high-visibility sight inserts

Markings/logos from the source photo are not required for the prototype and should be treated separately during final art/legal review.

## Runtime class

`ATU_G34CM : ATU_WeaponBase`

Prototype behavior:

- semi-auto only
- timed fire cadence
- timed reload lifecycle
- shared camera/viewpoint hit trace
- ADS vs hip spread
- recoil through the owning controller
- point damage dispatch
- shared attachment component
- shared `OnShotFired` presentation event

All numeric tuning is fictionalized game balance data, not a real-world performance specification.

## Suggested Unreal content layout

```text
/Game/Weapons/Pistols/G34CM/
  Meshes/
    SM_G34CM_Frame
    SM_G34CM_Slide
    SM_G34CM_Optic
    SM_G34CM_Magazine
    SM_G34CM_Magwell
    SM_G34CM_BarrelVisual
  Materials/
  Textures/
  Data/
    WPN_G34CM
    EQ_G34CM_Optic
    EQ_G34CM_ExtendedMagazine
    EQ_G34CM_Magwell
```

## Socket / animation intent

Author sockets from `G34CM_socket_manifest.json`.

Minimum presentation sockets/anchors:

- slide attachment root
- optic
- magazine
- magwell
- external barrel visual
- muzzle VFX point

The slide and magazine are intentionally separate so first-person firing and reload presentation can animate them later without replacing the whole weapon mesh.

The copper barrel model is external presentation geometry only.

## First-person acceptance

Before this pistol is considered production-ready:

1. import all split GLBs and verify scale/orientation
2. fit the frame to first-person hands
3. align the optic to the ADS camera
4. verify slide presentation has no clipping
5. verify magazine extraction/insertion path
6. test empty and tactical reload presentation
7. verify muzzle flash and impact event placement
8. verify recoil recovery feels distinct from the rifles
9. verify remote third-person weapon placement
10. add optimized collision and LODs
11. author final UV/PBR materials and surface wear
12. run `TheUnit.Combat.G34CM.Runtime`

## Shared architecture

The pistol intentionally reuses the same weapon runtime as the TU-556 and AK-105. `ATU_ArmedOperatorCharacter::DefaultWeaponClass` can point to `ATU_G34CM`, so no separate pistol input implementation is needed.
