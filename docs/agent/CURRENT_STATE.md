# Current State

Baseline: audited at `8988524a4ed45409df760ab668fdc9103bb4d231`; weapon state updated by TU-001; editor build and regression validated by TU-002. Unrelated systems have not been re-audited.

## Implemented foundations

- Unreal C++ project and `TheUnit` runtime module; PC prototype direction.
- Operator character with first-person camera/arms, walking, sprinting, crouching, ADS and lean state flags, legacy input bindings, and a placeholder interaction entry point.
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

- Target files use `BuildSettingsVersion.V6` and `EngineIncludeOrderVersion.Unreal5_7`, while `TheUnit.uproject` declares Engine Association `5.0`.
- Input uses legacy `InputCore`; no committed `Config/` mapping foundation exists.
- Module dependencies are currently `Core`, `CoreUObject`, `Engine`, and `InputCore`. Add others only for concrete implementations.
- Existing documentation is useful design context but can be stale: `README.md` describes an earlier phase, and `PHASE1_IMPLEMENTATION_GUIDE.md` incorrectly says project/module files still need generation.

## Weapon compatibility and build validation

- Existing actor action methods and ammo getters remain. Removed actor data properties and component `FireSemiAuto`/`Reload` Blueprint calls require migration to actor methods and `GetMagazineState`/`GetWeaponDefinition`/`GetAmmoDefinition` snapshots. Author defaults on the actor's native `WeaponMechanics` subobject. Fire rate now comes from definition RPM.
- Magazine rounds exclude the chamber. `GetCurrentAmmo` returns the loaded total: initially 29+1; tactical reload can reach 30+1. Empty reload loads 30 total. Every successful shot consumes exactly one round, including an initially empty chamber.
- All 11 TU-001 acceptance criteria checked: single magazine/reserve authority; one reload lifecycle and transfer path; one shot-consumption path; actor-owned fire modes/API; reused types; reflected Blueprint read/action API; no Tick; focused changes; affected Unreal compilation passes.
- TU-002 replaced exactly five unavailable `FLinearColor::Cyan` constants with explicit linear cyan in the Kill House and Stadium generators. Marker transforms, labels, and generator behavior are unchanged. The full `TheUnitEditor Win64 Development` build now compiles and links successfully using UE 5.7.4 explicitly.
- `TheUnit.Combat.WeaponOwnership` passed headlessly in the real repository project under UE 5.7.4: one test, zero errors or warnings. Coverage includes chamber/ammo conservation, reload boundaries, dry fire, fire-mode routing, reserve overflow, snapshots, and internal API ownership. Engine association remains stale; TU-003 is the next Wave 0 configuration task.

Update this file only when implementation or architecture state changes materially.
