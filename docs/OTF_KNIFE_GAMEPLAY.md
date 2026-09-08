# OTF Knife Gameplay + Asset Integration

## Scope

Adds a first-person OTF-style melee item to The Unit as a stacked branch on top of the shared firearm runtime.

The implementation models only game presentation and melee interaction. It does **not** model a real OTF knife's internal spring, lock, tolerances, mechanism geometry, or fabrication details.

## Runtime class

`ATU_OTFKnife`

Components:

- `Root`
- `HandleMesh`
- `BladeMesh`

The blade is a separate mesh and has four presentation states:

- Retracted
- Deploying
- Deployed
- Retracting

Equip behavior:

1. Attach the knife to the first-person arms socket.
2. Reset the blade to its authored retracted transform.
3. Animate the blade to the authored deployed transform over the configured game duration.
4. Allow melee hits only after the blade reaches `Deployed`.

Holster behavior:

1. Stop firearm interaction while the knife is active.
2. Animate the blade back to the retracted transform.
3. Hide the knife only after the retraction delay completes.
4. Restore the firearm visual/state.

Default prototype deploy/retract time: 0.12 s each. These are game-presentation values, not real mechanism timings.

## Operator integration

`ATU_ArmedOperatorCharacter` now owns an optional `DefaultMeleeClass` in addition to its firearm.

Prototype controls:

- `V`: draw / holster melee
- `Left Mouse`: melee sweep while the knife is equipped and fully deployed

While melee is equipped:

- firearm is hidden
- firearm firing/reload/fire-mode callbacks are suppressed
- ADS is not forwarded to the firearm
- left click routes to the melee sweep

The prototype reuses the existing `weapon_socket` for the knife so current arms do not require a new socket immediately. A dedicated melee socket can be authored later.

## Melee hit path

`PerformMeleeAttack()` performs a short first-person sphere sweep from the owning pawn's view direction.

On a hit it dispatches point damage through Unreal's standard damage path and broadcasts `OnMeleeHit` for presentation/audio hooks.

Current damage/range/radius values are fictionalized gameplay tuning and must be tuned in playtest.

## External art package

Chat-generated package: `OTFKnife_GameReady_Source.zip`

Files:

- `SM_OTFKnife_Handle.glb`
- `SM_OTFKnife_Blade.glb`
- `OTFKnife_Reference_Deployed.glb`
- `OTFKnife_Reference_Retracted.glb`
- `OTFKnife_manifest.json`

The manifest records the same retracted/deployed game transforms used as the initial Unreal target.

## Automation

`TheUnit.Combat.OTFKnife.Runtime`

Coverage:

- begins retracted
- independent handle and blade mesh components exist
- deploy enters deploying state
- deploy reaches fully deployed state
- retract enters retracting state
- retract reaches fully retracted state
- a retracted blade cannot perform a melee hit

## Validation gate

Keep the stacked PR draft until:

1. UE 5.7 UHT/C++ compile succeeds
2. `TheUnit.Combat.OTFKnife.Runtime` passes
3. parent firearm PR tests still pass
4. handle and blade GLBs import at the intended first-person scale
5. deployed/retracted transforms do not visibly clip through the hand/handle
6. deploy/retract sound and animation timing are tuned in PIE
7. melee trace origin/range feels correct in first person
8. weapon hiding/restoration is verified when rapidly toggling melee
9. multiplayer ownership/replication behavior is designed before network play
