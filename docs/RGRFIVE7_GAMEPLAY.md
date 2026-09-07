# RGR Five7 Gameplay / Art Integration

## Purpose

`ATU_RGRFive7` is a semi-auto 5.7-style tactical pistol profile built on the same shared runtime as the TU-556, AK-105 and G34 CM.

All numeric weapon/ammunition values are fictionalized gameplay balance values. The generated art package is external presentation geometry only and is not manufacturing/CAD data.

## Generated external source package

The chat-generated `RGRFive7_GameReady_Source.zip` contains:

- `SM_RGRFive7_Frame.glb`
- `SM_RGRFive7_Slide.glb`
- `SM_RGRFive7_Magazine.glb`
- `SM_RGRFive7_MuzzleDevice.glb`
- `SM_RGRFive7_BarrelVisual.glb`
- `RGRFive7_Reference_Assembled.glb`
- `RGRFive7_socket_manifest.json`
- `README.txt`

The supplied visual reference is represented as a long angular polymer-frame pistol with a removable slide, magazine, external barrel presentation piece and long suppressor-like muzzle attachment.

## Unreal content target

Suggested layout:

```text
/Game/Weapons/Pistols/RGRFive7/
  Meshes/
  Materials/
  Data/
  Animations/
```

Recommended imported names match the generated GLB names.

## Modular pieces

- Frame is the root weapon body.
- Slide uses the shared `Slide` attachment slot / animation boundary.
- Magazine uses the shared `Magazine` slot.
- Long suppressor-like visual uses the shared `MuzzleDevice` slot.
- Barrel presentation geometry uses the shared `BarrelVisual` slot.

Do not bake these pieces into one production mesh if they are expected to animate or be swapped in the armory.

## Gameplay profile

`ATU_RGRFive7` currently provides:

- semi-auto only
- timed fire cadence
- timed reload lifecycle
- independent recoil and spread values
- 20-round loaded total at spawn
- fictionalized `Ammo_TU57_Ball` profile
- shared ADS, hitscan, recoil, damage and attachment runtime

## First-person acceptance

Before marking production-ready:

1. Import and scale the split GLBs consistently.
2. Author sockets from `RGRFive7_socket_manifest.json`.
3. Fit the pistol to first-person hands.
4. Verify slide travel presentation does not clip the frame or sights.
5. Verify the magazine clears the grip during reload animation.
6. Verify the muzzle-device attachment remains aligned through recoil/ADS.
7. Verify iron-sight alignment at ADS.
8. Add muzzle flash/audio presentation at the final muzzle socket.
9. Add LODs and optimized collision.
10. Run `TheUnit.Combat.RGRFive7.Runtime` after UE 5.7 compile.

## Validation status

Code and automation coverage are authored, but this branch has not yet been compiled or executed in UE 5.7. Keep PR #19 draft until the shared weapon validation gate is complete.
