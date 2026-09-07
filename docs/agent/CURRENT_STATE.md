# Current State

Baseline: audited at `8988524a4ed45409df760ab668fdc9103bb4d231`; weapon state updated by TU-001; editor build and regression validated by TU-002; TU-003 aligns the project descriptor with UE 5.7; TU-004 commits the legacy input mapping foundation. Unrelated systems have not been re-audited.

## Implemented foundations

- Unreal C++ project and `TheUnit` runtime module; PC prototype direction.
- Operator character with first-person camera/arms, walking, sprinting, crouching, ADS and lean state flags, legacy input bindings, and a placeholder interaction entry point.
- `Config/DefaultInput.ini` now maps the operator's existing legacy movement/look, sprint, crouch, ADS, lean, and interact bindings. Fire/reload mappings are intentionally deferred until combat integration consumes them.
- Regional `UTUHealthComponent`, including lethal head/chest/total depletion and death notification, but not attached to player or enemy actors.
- TU-001: `ATU_WeaponBase` is the single external weapon actor/API. It owns fire modes, action routing, and reload lifecycle. Its private `UTUWeaponComponent` owns shared definitions, magazine/chamber/reserve state, atomic consumption, and reload transfer. There is no second callable component weapon API. Weapons remain disconnected from characters; ballistics, damage delivery, timing, recoil, equipment integration, and replication are absent.
- Shared weapon, ammunition, magazine, operator, gear, stance, body-region, and callout types in `TheUnitTypes.h`.
- Callout manager component with category cooldowns and subtitle events.
- Procedural two-story Kill House graybox with geometry and placeholder placement markers. Markers are not functional gameplay actors.
- Substantial Stadium generator, intentionally outside current priority.
- Git LFS rules for common Unreal/binary assets.

## Partial or disconnected

Health, weapons, operator combat, interaction, callouts, framework classes, objective/extraction architecture, multiplayer architecture, and Kill House gameplay markers require integration. Framework classes such as GameMode, GameState, PlayerController, PlayerState, objectives, and extraction are shells describing intended ownership—not completed gameplay.

## Not implemented

Actual hitscan/projectile combat, enemy AI/perception/patrol, inventory/loot/stash/persistence, functional objectives and extraction, Ready Room/loadouts, HUD/results UI, authoritative replication, and CI are absent. A focused weapon ownership automation test now exists; broader gameplay coverage is absent. Content directories are placeholder-only at the audited snapshot.

## Known issues and constraints

- Input remains on legacy `InputCore` intentionally for the current slice; the exact bindings consumed by `ATU_OperatorCharacter` are now committed in `Config/DefaultInput.ini`.
- Module dependencies are currently `Core`, `CoreUObject`, `Engine`, and `InputCore`. Add others only for concrete implementations.
- Existing documentation is useful design context but can be stale: `README.md` describes an earlier phase, and `PHASE1_IMPLEMENTATION_GUIDE.md` incorrectly says project/module files still need generation.

## Weapon compatibility and build validation

- Existing actor action methods and ammo getters remain. Removed actor data properties and component `FireSemiAuto`/`Reload` Blueprint calls require migration to actor methods and `GetMagazineState`/`GetWeaponDefinition`/`GetAmmoDefinition` snapshots. Author defaults on the actor's native `WeaponMechanics` subobject. Fire rate now comes from definition RPM.
- Magazine rounds exclude the chamber. `GetCurrentAmmo` returns the loaded total: initially 29+1; tactical reload can reach 30+1. Empty reload loads 30 total. Every successful shot consumes exactly one round, including an initially empty chamber.
- All 11 TU-001 acceptance criteria checked: single magazine/reserve authority; one reload lifecycle and transfer path; one shot-consumption path; actor-owned fire modes/API; reused types; reflected Blueprint read/action API; no Tick; focused changes; affected Unreal compilation passes.
- TU-002 replaced exactly five unavailable `FLinearColor::Cyan` constants with explicit linear cyan in the Kill House and Stadium generators. Marker transforms, labels, and generator behavior are unchanged. The full `TheUnitEditor Win64 Development` build compiled and linked successfully using UE 5.7.4 explicitly.
- `TheUnit.Combat.WeaponOwnership` passed headlessly in the real repository project under UE 5.7.4: one test, zero errors or warnings. Coverage includes chamber/ammo conservation, reload boundaries, dry fire, fire-mode routing, reserve overflow, snapshots, and internal API ownership.
- TU-003 changes only `TheUnit.uproject` Engine Association from `5.0` to `5.7`, matching the unchanged target settings.
- TU-004 adds only deterministic legacy input configuration; `ATU_OperatorCharacter` and `TheUnit.Build.cs` remain unchanged. This GitHub execution environment cannot run UnrealBuildTool or UnrealEditor, so the next executable UE environment must run the UE 5.7 editor build and weapon regression before accepting further gameplay-code changes.

Update this file only when implementation or architecture state changes materially.
