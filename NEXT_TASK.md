# TASK ID

TU-004

# OWNER

Character (QA validates configuration)

# TITLE

Commit the legacy input mapping foundation

# OBJECTIVE

Add the smallest committed Unreal input configuration required by the bindings that already exist in `ATU_OperatorCharacter`. Do not migrate input technology or add combat integration in this task.

# PREFLIGHT — CLOSE TU-003 VALIDATION

Before editing, run the UE 5.7 editor build and existing weapon regression against the updated project descriptor:

- `Build.bat TheUnitEditor Win64 Development -Project=<absolute TheUnit.uproject> -WaitMutex -NoHotReloadFromIDE`
- `UnrealEditor-Cmd.exe <absolute TheUnit.uproject> /Engine/Maps/Entry -unattended -NullRHI -nosound -ExecCmds="Automation RunTests TheUnit.Combat.WeaponOwnership" -TestExit="Automation Test Queue Empty"`

If either fails because of the TU-003 descriptor change, stop and report it. Do not continue into input work until the project still builds and the regression passes.

# REQUIRED CONTEXT

Read only:

- `Source/TheUnit/Private/TU_OperatorCharacter.cpp` input-binding section
- `Source/TheUnit/TheUnit.Build.cs`
- `.gitignore`
- `docs/agent/CODING_RULES.md`
- `docs/agent/CURRENT_STATE.md` input/config note

Do not audit the repository, inspect Content, or read full design documents. Inspect additional files only for a concrete build/config failure.

# IMPLEMENTATION DIRECTION

Create `Config/DefaultInput.ini` if the project still has no committed input configuration. Map the exact legacy names already consumed by `ATU_OperatorCharacter`:

Axes:
- `MoveForward`: W = +1, S = -1
- `MoveRight`: D = +1, A = -1
- `Turn`: MouseX = +1
- `LookUp`: MouseY = -1

Actions:
- `Sprint`: LeftShift
- `Crouch`: LeftControl
- `ADS`: RightMouseButton
- `LeanLeft`: Q
- `LeanRight`: E
- `Interact`: F

Use standard `/Script/Engine.InputSettings` legacy mappings. Keep the file minimal and deterministic. Do not add unused fire/reload bindings yet because the operator does not consume them.

# NON-GOALS

No Enhanced Input migration, gameplay code changes, weapon integration, fire/reload controls, AI, UI, map/content edits, module-dependency changes, project regeneration, or unrelated cleanup.

# ACCEPTANCE CRITERIA

- `Config/DefaultInput.ini` is committed and maps every legacy action/axis currently bound by `ATU_OperatorCharacter`.
- Mapping names exactly match the source bindings.
- No currently unused combat mappings are added.
- `ATU_OperatorCharacter` source remains unchanged.
- `TheUnit.Build.cs` remains unchanged.
- `TheUnitEditor Win64 Development` builds under UE 5.7.
- `TheUnit.Combat.WeaponOwnership` still passes.
- No generated files, binaries, or unrelated changes are committed.

# VALIDATION

After adding the config, rerun the UE 5.7 editor build and `TheUnit.Combat.WeaponOwnership`. Review `git diff`, run `git diff --check`, and confirm only the config plus necessary agent-state/next-task handoff changed.

After successful validation, the next task must begin the mandatory modular weapon foundation before playable combat integration: weapon platforms, part slots, compatibility metadata, and fire-control/trigger-module ownership. Do not skip directly to player weapon firing.
