# TASK ID

TU-003

# OWNER

Lead / Architect (QA validates configuration)

# TITLE

Align project engine association with UE 5.7

# OBJECTIVE

Complete the smallest engine-association/configuration cleanup from Wave 0 of `docs/agent/ROADMAP.md`: align the stale project association with the existing UE 5.7 target settings.

# REQUIRED CONTEXT

- `TheUnit.uproject`
- `Source/TheUnit.Target.cs`
- `Source/TheUnitEditor.Target.cs`
- `docs/agent/ARCHITECTURE.md` (engine configuration paragraph only)
- `docs/agent/CURRENT_STATE.md` (known engine issue and validation notes only)
- `docs/agent/CODING_RULES.md`

Do not audit source, inspect Content, or read full design documentation. Read additional files only for a concrete build/test failure.

# IMPLEMENTATION DIRECTION

Confirm the targets use `BuildSettingsVersion.V6` and `EngineIncludeOrderVersion.Unreal5_7`. Change only `EngineAssociation` in `TheUnit.uproject` from `5.0` to `5.7`. Keep the shared version string, not a machine-specific engine path or installation GUID. Update the two agent documents only where they describe this now-resolved discrepancy.

# NON-GOALS

No gameplay, input mappings, target-setting changes, module dependencies, project regeneration, content/binaries, CI setup, or broad documentation cleanup.

# ACCEPTANCE CRITERIA

- Project descriptor remains valid JSON with `EngineAssociation` equal to `5.7`.
- Existing target settings remain unchanged and consistent with that association.
- Agent architecture/current-state notes no longer describe the association as stale.
- `TheUnitEditor Win64 Development` builds using UE 5.7.
- `TheUnit.Combat.WeaponOwnership` passes in the real project.
- Diff contains only the descriptor value, necessary agent-state documentation, and the next-task handoff; no generated files or binaries.

# VALIDATION

Parse the descriptor as JSON. Run UE 5.7 `Build.bat TheUnitEditor Win64 Development -Project=<absolute TheUnit.uproject> -WaitMutex -NoHotReloadFromIDE`.

Run `UnrealEditor-Cmd.exe <absolute TheUnit.uproject> /Engine/Maps/Entry -unattended -NullRHI -nosound -ExecCmds="Automation RunTests TheUnit.Combat.WeaponOwnership" -TestExit="Automation Test Queue Empty"`; inspect the result.

Review `git diff` and `git diff --check`, verify the target files are unchanged, and keep generated output out of Git. After validation, select the next smallest input/config foundation task from the roadmap.
