# Karambit gameplay / asset integration

## Purpose

Adds a fixed-blade karambit-style melee option to The Unit's first-person melee runtime.

This is external game content only. The generated source package is not manufacturing/CAD data and contains no real-world use instruction.

## Runtime class

`ATU_Karambit` derives from the current melee implementation (`ATU_OTFKnife`) so it reuses the same operator equip/holster and melee-hit path while PR #20 remains small and reviewable.

The difference is presentation:

- the OTF blade visibly travels between retracted and deployed transforms;
- the karambit uses identical authored blade transforms, so the blade remains fixed;
- drawing only changes visibility/readiness after a short presentation delay;
- holstering clears readiness before the item is hidden;
- melee attacks remain unavailable while holstered.

`ATU_ArmedOperatorCharacter::DefaultMeleeClass` can point to either `ATU_OTFKnife` or `ATU_Karambit`, so no duplicate input implementation is required.

## External source package

Chat-generated package:

- `SM_Karambit_Handle.glb`
- `SM_Karambit_Blade.glb`
- `Karambit_Reference_Assembled.glb`
- `Karambit_manifest.json`

Reference traits represented in the blockout:

- strongly curved fixed blade
- black textured ergonomic handle
- pronounced guard
- rear finger ring
- separate handle/blade meshes for later art iteration

## Unreal import target

Suggested content layout:

```text
Content/TheUnit/Weapons/Melee/Karambit/
  Meshes/
    SM_Karambit_Handle
    SM_Karambit_Blade
  Blueprints/
    BP_Karambit
  Materials/
```

Suggested Blueprint setup:

1. Create `BP_Karambit` derived from `ATU_Karambit`.
2. Assign `SM_Karambit_Handle` to `HandleMesh`.
3. Assign `SM_Karambit_Blade` to `BladeMesh`.
4. Fit the actor to the first-person `weapon_socket` (or a dedicated melee socket later).
5. Set the operator's `DefaultMeleeClass` to `BP_Karambit` for a karambit-equipped loadout.

## Prototype controls

The karambit uses the same PR #20 melee controls:

- `V`: draw / holster melee
- `Left Mouse`: melee attack while the item is equipped and ready

## Validation gate

Before merge/production acceptance:

1. UE 5.7 UHT/C++ compile succeeds.
2. `TheUnit.Combat.OTFKnife.Runtime` remains green.
3. `TheUnit.Combat.Karambit.Runtime` passes.
4. GLBs import at correct first-person scale.
5. Finger ring and guard do not clip the hand pose.
6. Fixed blade does not visibly translate during draw/holster.
7. Draw/holster timing is tuned in PIE.
8. First-person draw and slash/stab presentation animations are authored.
9. Audio and impact presentation are authored.
10. Multiplayer ownership/replication is designed before network play.
