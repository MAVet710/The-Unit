# TASK ID

TU-005

# OWNER

Combat / Lead Architecture (QA validates)

# TITLE

Define modular weapon platform and part data model

# OBJECTIVE

Begin Wave 0.5 by adding the smallest compile-safe, data-driven definitions needed to represent a weapon platform and compatible installed parts. This task establishes customization vocabulary only; it does not yet mutate runtime weapon builds or change firing behavior.

# PREFLIGHT

Before editing in an environment with UE 5.7 available:

1. Build `TheUnitEditor Win64 Development` against the current `TheUnit.uproject`.
2. Run `TheUnit.Combat.WeaponOwnership`.

If either fails because of TU-003/TU-004, stop and fix only that regression before continuing.

# REQUIRED CONTEXT

Read only:

- `Source/TheUnit/Public/TheUnitTypes.h`
- `Source/TheUnit/Public/TU_WeaponBase.h`
- `Source/TheUnit/Public/TUWeaponComponent.h`
- `docs/agent/ARCHITECTURE.md` modular weapon section
- `docs/agent/GAMEPLAY_CONTRACTS.md` weapon contracts
- `docs/agent/CODING_RULES.md`
- `docs/agent/ROADMAP.md` Wave 0.5 only

Do not audit unrelated source or Content.

# IMPLEMENTATION DIRECTION

Extend the canonical shared type surface in `TheUnitTypes.h` with a data-only modular weapon model. Do not create per-weapon subclasses.

Add a Blueprint-visible weapon-part slot/category enum covering the gameplay customization surfaces already mandated by architecture, including at minimum:

- Barrel
- Muzzle
- Operating/Action component
- Handguard
- Stock/Brace
- Grip
- Magazine/Feed
- Optic
- OpticMount
- Rail/Mount
- LightLaser
- Underbarrel
- Internal
- FireControl
- Cosmetic

Keep the receiver/platform represented by the platform definition rather than an installable part slot in this first task.

Add compact data-table-friendly definitions for:

- `FWeaponPlatformDefinition`
  - stable platform ID
  - display name
  - compatibility/interface tags as `FName` values
  - supported part slots
  - compatible ammunition identifiers/tags

- `FWeaponPartDefinition`
  - stable part ID
  - display name
  - slot/category
  - required compatibility/interface tags
  - provided compatibility/interface tags
  - generic gameplay modifier fields only where already justified by the existing weapon model (for example recoil/spread/fire-rate modifiers)

- `FWeaponBuildState`
  - platform ID
  - installed part IDs keyed by slot
  - selected ammunition ID if useful for composition identity

Compatibility metadata must remain abstract/data-driven. Do not model real-world assembly procedures or mechanical installation steps.

Do not move fire modes yet. Fire-control behavior gets its own follow-up task after these definitions exist.

# NON-GOALS

- No player integration.
- No hitscan/projectiles/damage delivery.
- No runtime install/remove API yet.
- No inventory persistence yet.
- No UI.
- No new assets.
- No GameplayTags module dependency unless the existing minimal `FName` compatibility representation proves insufficient.
- No change to current fire/reload behavior.
- No physical weapon-construction simulation.

# ACCEPTANCE CRITERIA

- Shared types compile under UE 5.7.
- Platform, part, slot, and build-state definitions are Blueprint/data-table friendly where appropriate.
- Stable IDs and compatibility metadata are explicit.
- FireControl exists as a first-class part slot.
- Existing `FWeaponDefinition`, `FAmmoDefinition`, and `FMagazineState` remain source compatible.
- No duplicate mutable ammunition or firing state is introduced.
- No new module dependency is required unless justified.
- Existing `TheUnit.Combat.WeaponOwnership` passes unchanged.
- Add a focused automation test for default construction/identity/slot semantics if it can be done without testing Unreal reflection internals.
- No unrelated files or binaries change.

# VALIDATION

Build `TheUnitEditor Win64 Development`, run `TheUnit.Combat.WeaponOwnership`, and run any new modular-definition test. Review `git diff` and `git diff --check`.

After success, select TU-006 to implement compatibility evaluation and runtime weapon-build composition. Fire-control/trigger behavior remains a mandatory dedicated task immediately after the composition foundation.
