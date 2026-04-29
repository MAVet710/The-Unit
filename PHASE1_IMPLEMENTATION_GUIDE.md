# The Unit - Phase 1 UE5 Implementation Guide

This repository now contains **C++ foundations** for Phase 1 and a **Blueprint build plan** for a playable graybox tactical PvE prototype.

## Folder Layout

- `/Source/TheUnit` (C++ systems)
- `/Content/TheUnit/Blueprints`
- `/Content/TheUnit/Characters`
- `/Content/TheUnit/Weapons`
- `/Content/TheUnit/AI`
- `/Content/TheUnit/UI`
- `/Content/TheUnit/Maps`
- `/Content/TheUnit/Data`
- `/Content/TheUnit/Audio`
- `/Content/TheUnit/Materials`
- `/Content/TheUnit/DevTest`

## C++ Added

- `TheUnitTypes.h`
  - `FWeaponDefinition`, `FAmmoDefinition`, `FMagazineState`
  - `FOperatorProfile`, `FGearSlot` (enum as `ETUGearSlot`), `FPatchAnchor` placeholders for Phase 2
  - Stance/body/callout enums
- `TUWeaponComponent`
  - Semi-auto firing
  - Magazine + reserve reload logic
  - Chambered round support
- `TUHealthComponent`
  - Regional body-part health (Head, Chest, Stomach, Arms, Legs)
  - Death on Head/Chest/Total depletion
  - Placeholder states (bleeding, limping, heavy breathing, suppressed)
- `TUCalloutManagerComponent`
  - Category cooldown throttling
  - Subtitle event broadcast for HUD

## Blueprint Assets To Create In Editor (Phase 1 Playable)

> These are required runtime assets and should be authored in Unreal Editor.

### Characters
- `BP_TU_PlayerCharacter` (inherits Character)
  - Add components: `TUWeaponComponent`, `TUHealthComponent`, `TUCalloutManagerComponent`
  - Implement: walk/sprint/crouch, ADS, low-ready, interact, reload, fire
  - Stub prone + lean as bool/axis state if full animation not ready
- `BP_TU_PlayerController`
- `BP_TU_GameMode_Phase1`

### Weapons
- `BP_TU556_TrainingRifle`
  - Hook muzzle flash Niagara placeholder
  - Hook impact decal/particle placeholder
  - Hook recoil camera kick values from `FWeaponDefinition`

### AI
- `BP_HostileRifleman`
  - Add `TUWeaponComponent`, `TUHealthComponent`, `TUCalloutManagerComponent`
  - Variables: DetectionRange, HearingRange, Accuracy, FireRate, Morale, SuppressionLevel, ReactionDelayRange
- `BB_HostileRifleman`
- `BT_HostileRifleman`
- `BTT_FireAtTarget`, `BTT_ReloadIfNeeded`, `BTT_CalloutContact`

### Objective & Extraction
- `BP_Objective_SecureIntelPackage`
  - 3-second hold interact
  - on complete -> callout "Objective secure."
- `BP_ExtractionZone`
  - Locked until objective complete
  - 5-second hold interact when active
  - Emits "Extraction unavailable." / "Hold to extract."

### UI
- `WBP_TU_HUD`
  - Ammo/Mag count
  - Health status (region summary ok for phase 1)
  - Objective + extraction status
  - Interaction prompt
  - Callout subtitle area
- `WBP_RaidSuccess`
- `WBP_RaidFailed`

### Map
- `M_TheUnit_Phase1_Graybox`
  - Spawn staging room
  - Outdoor lane
  - 5-room compound
  - Objective room
  - Extraction volume
  - Patrol route points
  - Basic gray materials and debug labels

## Raid Flow

1. Start from debug menu or PIE in `M_TheUnit_Phase1_Graybox`
2. Player spawns with TU-556 Training Rifle
3. Enemy patrols and engages
4. Player secures intel objective (3s hold)
5. Extraction zone becomes live
6. Player extracts (5s hold) -> success screen
7. If player dies -> failed raid screen

## Known Limitations (Current Repo State)

- Unreal `.uproject`, module build files, and `.uasset` Blueprints are not generated in this repository snapshot.
- AI behavior tree and map geometry must be authored in UE5 editor.
- Prone/lean are intended as phase-1 stubs unless animation setup is available.

## Phase 2 Next Build Targets

- Inventory/stash persistence
- Attachment sockets and compatibility rules
- Armor zones and plate logic
- Expanded callout VO asset routing
- Operator outfit and patch anchor runtime system
