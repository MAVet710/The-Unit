# Melee equipment slot and selection

PR #20 now owns one explicit first-person melee equipment slot rather than requiring `DefaultMeleeClass` to be changed manually in a Blueprint.

## Runtime architecture

`UTUMeleeLoadoutComponent` owns the selection state. Each `FTUMeleeEquipmentEntry` contains:

- stable `ItemId`
- display name
- runtime melee actor class
- first-person equip socket
- inventory weight

The component stores available items and the selected item. It does not spawn actors. `ATU_ArmedOperatorCharacter` owns the runtime actor and respawns it when the selected melee item changes.

Built-in prototype choices:

| Item id | Class | Weight | Presentation |
| --- | --- | ---: | --- |
| `MELEE_OTF` | `ATU_OTFKnife` | 0.14 kg | retracts/deploys blade on holster/draw |
| `MELEE_Karambit` | `ATU_Karambit` | 0.18 kg | fixed blade; short readiness transition |

The weights are inventory/gameplay values and can be retuned. `DefaultMeleeClass` remains only as a backward-compatible fallback for an empty or invalid loadout.

## Player controls

- `V`: draw / holster the selected melee item
- `N`: cycle the selected melee item while holstered
- `Left Mouse`: melee attack while the selected melee item is drawn and attack-ready

Selection changes are rejected while melee is drawn or currently holstering. This prevents destroying/swapping a live melee actor during its presentation state.

## Armory / loadout UI integration

The runtime selection boundary is complete. A future armory screen does not need melee-specific subclasses hard-coded. It can:

1. call `GetMeleeLoadout()` on the operator
2. read `GetAvailableItems()`
3. render display name / weight / future icon metadata
4. call `SelectMeleeById(ItemId)`
5. show `GetSelectedMeleeId()` as the active melee slot choice
6. read `GetSelectedMeleeWeightKg()` for encumbrance/loadout totals

`SetItems()` allows a game mode, progression system, saved loadout, or inventory layer to replace the available melee list and preserve a preferred selection.

## Extending the slot

A future melee item only needs to derive from the current melee runtime base boundary (`ATU_OTFKnife` for this prototype), then be added as an `FTUMeleeEquipmentEntry`. No new input binding or operator fire-routing code is required.

The current inheritance is intentionally a prototype compromise: `ATU_Karambit` inherits the existing OTF melee runtime with fixed blade transforms. A later cleanup may split the common hit/equip behavior into a neutral `ATU_MeleeWeaponBase`, but that refactor is not required for loadout selection to work.

## Automation

`TheUnit.Combat.Melee.Loadout` verifies:

- built-in OTF + karambit inventory
- default OTF selection
- selection by stable item id
- forward/backward wrap-around cycling
- custom item registration
- duplicate-id replacement without duplicate inventory entries
- selected weight updates
- full inventory replacement / preferred selection
- rejection of unknown ids

`TheUnit.Combat.Melee.OperatorSelection` verifies:

- the operator owns the melee loadout component
- the OTF runtime actor is spawned from the selected slot
- selecting karambit replaces the holstered runtime actor
- selection changes propagate weight to the operator-facing API
- cycling replaces the runtime actor back with OTF
- drawing marks melee active
- direct selection and cycling are both blocked while melee is active

## Validation gate

The selection architecture is implemented, but PR #20 stays draft until runtime validation:

1. UE 5.7 UHT/C++ compile succeeds
2. all `TheUnit.Combat.Melee.*`, OTF and karambit automation passes
3. parent PR #19 combat automation remains green
4. `N` cycling is verified in PIE while holstered
5. selection is confirmed blocked during draw/holster states
6. both external GLB packages fit the first-person hand/socket
7. OTF deploy/retract and fixed-blade karambit presentation are visually validated
8. draw/attack/holster audio + first-person animation are authored or intentionally deferred
9. saved-loadout persistence and multiplayer replication are designed before networked progression is enabled
