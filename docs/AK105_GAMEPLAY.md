# AK-105 Gameplay / Asset Integration

## Goal

Add the supplied modernized AK-105 visual configuration as a second playable modular rifle using the same runtime introduced for the TU-556.

The implementation is game-facing only. The generated art represents external visual geometry and attachment layout, not functional firearm internals or manufacturing/CAD data.

## Runtime class

`ATU_AK105` subclasses `ATU_WeaponBase` and opts into the shared timed weapon runtime:

- semi-auto and full-auto input
- RPM-derived fire cadence
- timed reload
- hip-fire / ADS spread
- controller recoil
- hitscan impact/damage dispatch
- `OnShotFired` presentation hook
- shared `UTUWeaponAttachmentComponent`

Its damage/recoil/ammunition values are prototype gameplay tuning and should be balanced in-game rather than treated as measured real-world performance.

## Visual target from the supplied reference

The reference build is represented as:

- compact AK-pattern receiver/body
- black receiver, pistol grip and curved magazine
- tan modern railed handguard/chassis
- tan/black adjustable stock assembly
- compact tube-style optic
- long muzzle-device visual

## Generated source package

The chat-generated `AK105_GameReady_Source.zip` contains:

```text
SM_AK105_Body.glb
SM_AK105_Stock.glb
SM_AK105_Magazine.glb
SM_AK105_Muzzle.glb
SM_AK105_Optic.glb
AK105_Reference_Assembled.glb
AK105_socket_manifest.json
README.txt
```

The pieces are deliberately split so Unreal can use the same attachment/loadout system as the TU-556 rather than importing one welded prop.

## Suggested Unreal destination

```text
/Game/Weapons/Rifles/AK105/
  Meshes/
  Materials/
  Attachments/
  Data/
```

Suggested assets:

- `SM_AK105_Body`
- `SM_AK105_Stock_Reference`
- `SM_AK105_Magazine_Reference`
- `SM_AK105_Muzzle_Reference`
- `SM_AK105_Optic_Reference`
- attachment data assets for Stock, Magazine, MuzzleDevice and Optic
- `WL_AK105_Reference` as `UTUWeaponLoadoutData`

## Socket / attachment intent

Author the final sockets from `AK105_socket_manifest.json` after importing and visually validating the body mesh:

- `stock_socket`
- `magazine_socket`
- `muzzle_device_socket`
- `optic_socket`
- `Muzzle` for gameplay/VFX origin

Socket transforms are blockout-fit starting points only; first-person fitting in Unreal is authoritative.

## Required production pass

Before the rifle is considered production-ready:

1. UE 5.7 compile/UHT validation.
2. Run `TheUnit.Combat.AK105.Runtime` plus existing weapon tests.
3. Import generated GLBs and validate X-forward scale/orientation.
4. Refine body/handguard/stock proportions against the reference image.
5. Replace blockout face colors with authored UV/PBR material sets.
6. Add first-person and third-person animation handling.
7. Align optic centerline for ADS.
8. Validate magazine hand placement/reload animation.
9. Add muzzle flash, sound, casing/impact presentation through `OnShotFired`.
10. Add LODs and optimized collision.
11. Test swaps with alternate optics, muzzle devices, stocks and magazines.
12. Validate multiplayer replication when the weapon runtime reaches its network pass.

## Branch relationship

This work stays in weapon PR #19 (`feature/tactical-rifle-gameplay-core`). It reuses the generic weapon/attachment implementation rather than creating a second weapon architecture.
