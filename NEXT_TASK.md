# TASK ID

TU-002

# OWNER

World (QA validates the build)

# TITLE

Fix UE 5.7 generator color compile errors

# OBJECTIVE

Restore the editor build blocked by five existing `FLinearColor::Cyan` references. This narrow prerequisite comes before engine-association/configuration cleanup. Do not redesign either generator.

# REQUIRED CONTEXT

Read only the compiler-identified marker lines and their immediate context in:

- `Source/TheUnit/Private/TU_KillhouseGenerator.cpp` (lines 82-83 at task creation)
- `Source/TheUnit/Private/TU_StadiumGenerator.cpp` (lines 263-265 at task creation)
- `docs/agent/CODING_RULES.md`
- `docs/agent/GAMEPLAY_CONTRACTS.md`

Only inspect additional code when a concrete compiler error requires it. Do not audit either generator or scan Content.

# IMPLEMENTATION DIRECTION

Replace the five unavailable `FLinearColor::Cyan` constants with explicit linear cyan, `FLinearColor(0.0f, 1.0f, 1.0f, 1.0f)`. Preserve marker transforms, labels, layout, and all gameplay behavior. No helper abstraction or dependency is needed.

# NON-GOALS

No weapon changes, gameplay integration, map/content edits, generator redesign, networking, engine-association changes, or unrelated cleanup. Stadium changes are limited to its three compiler-reported color constants.

# ACCEPTANCE CRITERIA

- All five invalid constants replaced with the same intended cyan color.
- Source diff contains only those five replacements.
- `TheUnitEditor Win64 Development` builds using UE 5.7 explicitly, despite the separately known stale project association.
- `TheUnit.Combat.WeaponOwnership` passes in the real project.
- No generated files, binaries, or unrelated changes committed.

# VALIDATION

Use UE 5.7 `Build.bat TheUnitEditor Win64 Development -Project=<absolute TheUnit.uproject> -WaitMutex -NoHotReloadFromIDE`.

Run `UnrealEditor-Cmd.exe <absolute TheUnit.uproject> /Engine/Maps/Entry -unattended -NullRHI -nosound -ExecCmds="Automation RunTests TheUnit.Combat.WeaponOwnership" -TestExit="Automation Test Queue Empty"` and inspect the test result.

Review the targeted diff and `git diff --check`. Report newly exposed, unrelated compile failures separately. Update material current state and select engine-association/config cleanup only after this task validates.
