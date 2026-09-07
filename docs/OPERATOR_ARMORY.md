# Operator armory and loadout UI

This stacked branch adds a player-facing armory above PR #19 firearms and PR #20 melee selection.

## Runtime slots

The armed operator now owns these explicit loadout slots:

- Primary
- Secondary
- Melee
- Equipment

`UTUOperatorLoadoutComponent` owns Primary, Secondary and Equipment selection. `UTUMeleeLoadoutComponent` from PR #20 remains the source of truth for the Melee slot.

### Built-in Primary choices

- `PRIMARY_TU556` -> `ATU_TacticalRifle`
- `PRIMARY_AK105` -> `ATU_AK105`
- `PRIMARY_M110` -> `ATU_M110`

### Built-in Secondary choices

- `SECONDARY_G34CM` -> `ATU_G34CM`
- `SECONDARY_RGRFIVE7` -> `ATU_RGRFive7`

### Built-in Melee choices

- `MELEE_OTF` -> `ATU_OTFKnife`
- `MELEE_Karambit` -> `ATU_Karambit`

### Equipment boundary

The Equipment slot currently exposes `EQUIPMENT_EMPTY` only. The purpose is to establish the loadout/UI/save-data boundary before concrete equipment actor classes such as FPV drones, breaching tools, cameras, shields or mission gadgets are connected.

## Runtime firearm behavior

Primary and Secondary are separate spawned weapon actors. The inactive firearm is hidden rather than destroyed.

That means switching between 1 and 2 preserves each runtime actor's own:

- magazine state
- reserve ammunition
- reload state
- fire mode
- attachment state

Changing a loadout selection replaces only that selected slot's actor. Selecting a new secondary must not replace/reset the primary, and vice versa.

## Prototype controls

- `1`: equip Primary
- `2`: equip Secondary
- `V`: draw / holster selected Melee
- `N`: cycle Melee while holstered
- `L`: open / close Armory
- Left Mouse: firearm fire or melee attack depending on active presentation
- Right Mouse: firearm ADS when armory/melee are not active

## Native armory UI

`UTUArmoryWidget` is a native `UUserWidget` backed by Slate. The current prototype deliberately avoids requiring authored Widget Blueprint assets so it can be validated from C++ first.

The screen renders four sections:

1. Primary
2. Secondary
3. Melee
4. Equipment

Each entry shows its display name, selected state and fictionalized inventory weight. A summary shows the active firearm slot and selected carried loadout weight.

Selection applies immediately while the Armory is open. Opening the Armory stops firearm fire/ADS, exposes the mouse cursor and moves the local player to Game+UI input mode. Closing returns to Game input mode.

The Armory cannot open while melee is drawn/holstering, and weapon/melee combat input is suppressed while it is open.

## Armory-facing API

`ATU_ArmedOperatorCharacter` exposes:

- `GetOperatorLoadout()`
- `GetMeleeLoadout()`
- `GetPrimaryWeapon()`
- `GetSecondaryWeapon()`
- `GetCurrentWeapon()`
- `GetActiveWeaponSlot()`
- `SelectPrimaryById()`
- `SelectSecondaryById()`
- `SelectMeleeById()`
- `SelectEquipmentById()`
- `EquipWeaponSlot()`
- `GetSelectedLoadoutWeightKg()`
- `OpenArmory()` / `CloseArmory()` / `ToggleArmory()`

This is the intended boundary for a later polished armory screen, saved loadouts, progression/unlocks and multiplayer replication.

## Automation added

- `TheUnit.Combat.OperatorLoadout.Data`
- `TheUnit.Combat.OperatorLoadout.RuntimeSlots`

Coverage includes built-in inventory, default selections, invalid-id rejection, Primary/Secondary runtime spawning, slot switching without replacing the opposite actor, changing an active secondary to RGR Five7, changing primary to M110, and selected loadout weight aggregation.

## Validation gate

This code is not considered production-ready until:

1. UE 5.7 UHT/C++ compile succeeds.
2. Existing PR #19 and PR #20 automation remains green.
3. `TheUnit.Combat.OperatorLoadout.*` executes and passes.
4. 1/2 switching is verified in PIE with ammo state preserved.
5. L opens/closes the native Armory with correct mouse/input ownership.
6. Clicking Primary/Secondary/Melee choices changes only the intended runtime slot.
7. Fire/ADS/reload cannot leak through the Armory UI.
8. The Armory is gated to prep/safe contexts before production gameplay.
9. First-person weapon draw/holster and hand-pose animation assets are authored.
10. Saved-loadout persistence and multiplayer replication are designed and tested.

## Next presentation pass

After compile/PIE validation, the next layer should connect first-person presentation assets through explicit hooks for:

- primary draw / holster
- secondary draw / holster
- OTF hand pose + thumb switch + deploy sound
- karambit draw / fixed-blade hand pose
- melee attack montages
- firearm reload montages
- muzzle/impact/audio presentation

The runtime slot architecture should not need another rewrite for those assets.
