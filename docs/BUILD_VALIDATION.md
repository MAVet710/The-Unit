# UE 5.7 build validation

Status: **BLOCKED / NOT EXECUTED**. Milestone A is not satisfied.

## Environment

- Host: Linux x86_64, kernel 6.18.35.
- Target required: `TheUnitEditor Win64 Development`.
- Source base inspected: `1c39edbf916e4c6d7ddcf8ccb6c6cdfd8fee4ab6`.
- Work branch: `agent/build` for `integration/working-game-v1`.
- Project engine association: `5.7`.
- Target files specify `BuildSettingsVersion.V6` and `EngineIncludeOrderVersion.Unreal5_7`.
- UE editor, UnrealBuildTool, UHT, Windows MSVC compiler and Win64 SDK: unavailable in this execution environment.

## Executed checks

`command -v UnrealEditor UnrealEditor-Cmd UnrealBuildTool RunUAT.sh Build.bat cl.exe pwsh` returned no executable paths.

A file search under `/opt`, `/usr/local`, `/workspace`, and `/mnt` for `UnrealEditor`, `UnrealEditor.exe`, `Build.bat`, `UnrealBuildTool.dll`, and `RunUAT.bat` returned no files.

A source-only generated-header ordering scan found no reflected header with another include after its generated header. This is not UHT execution.

The module declares Core, CoreUObject, Engine, InputCore, UMG, Slate and SlateCore. No speculative module additions or engine setting changes were made without a compiler diagnostic.

## Focused include fixes

- `TUCalloutManagerComponent.cpp`: explicitly include `Engine/World.h` for `GetWorld()->GetTimeSeconds()`.
- `TU_ExtractionZone.cpp`: explicitly include `Engine/World.h` for `GetWorld()->GetTimerManager()`.
- `TU_ArmedOperatorCharacter.h`: include `TUBriefingWidget.h` so inline `IsValid(BriefingWidget)` has a complete widget type when converting it to UObject. The widget header only forward-declares the operator, so this creates no include cycle.

These remove dependencies on unity-build/transitive includes. Their effect has not been verified by a UE compiler. Gameplay behavior is unchanged.

## Blocker record

BLOCKER: Required UE 5.7 Windows build environment unavailable.

SYSTEM: Build / UHT / Win64 toolchain.

ERROR: No build process could be started because engine build tools and Windows compiler are absent. There is no Unreal compiler diagnostic to report.

LOG: Executable lookup and filesystem search above; no Unreal build log exists.

ROOT CAUSE: The provided host is Linux with no discoverable Unreal installation or connected Windows build runner.

FIX: Source include corrections only. This does not resolve the environment blocker.

RETEST: Not executed; UE 5.7 editor build, UHT, automation and runtime remain unverified.

## Required validation command

Run on an available Windows UE 5.7 machine from the integration checkout:

```powershell
& "$UnrealRoot\Engine\Build\BatchFiles\Build.bat" TheUnitEditor Win64 Development "$ProjectRoot\TheUnit.uproject" -WaitMutex -NoHotReloadFromIDE
```

Do not mark build successful without the process exit code and complete Unreal build log. Downstream integration remains gated by this result.
