# The Unit

A tactical co-op PvE extraction shooter prototype built in **Unreal Engine 5**.

## Project Status

This repository has completed **Phase 0: Foundation** and is now in **Phase 1A: UE5 C++ Project Skeleton**.

Phase 1A goals:
- Initialize the UE5 C++ project definition.
- Establish core framework gameplay class stubs.
- Keep systems minimal and compile-safe for future iteration.

## Core Vision (Summary)

- Squad-based tactical PvE with meaningful risk/reward.
- Session-based extraction loop with persistent progression.
- Strong emphasis on communication, positioning, and encounter planning.

For full design context, see:
- `THE_UNIT_TACTICAL_PVE_EXTRACTION_DESIGN_REPORT.md`

## Repository Layout

- `TheUnit.uproject` — UE5 project file.
- `Source/TheUnit/` — C++ gameplay module root.
- `Content/TheUnit/` — UE5 content root for project assets.
- `docs/` — planning and technical documentation.

## How to Open the Project in Unreal Engine 5

1. Install a compatible Unreal Engine 5 version via Epic Games Launcher.
2. From the project root, double-click `TheUnit.uproject`.
3. If prompted, let Unreal build missing project files/modules.

## How to Generate Visual Studio Project Files

### Option A: Windows Explorer context menu
1. Right-click `TheUnit.uproject`.
2. Select **Generate Visual Studio project files**.

### Option B: UnrealBuildTool command line
Run from the repository root (adjust the engine path to your local install):

```powershell
"C:\Program Files\Epic Games\UE_5.x\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="<ABSOLUTE_PATH>\TheUnit.uproject" -game -rocket -progress
```

## Phase 1A Status

✅ Completed:
- `TheUnit.uproject` added.
- Base runtime C++ module files added (`TheUnit.Build.cs`, `TheUnit.cpp`, `TheUnit.h`).
- Initial gameplay class stubs added for framework, player, mission, extraction, interaction, and callouts.

🔜 Next:
- Begin implementing baseline gameplay loop behavior in Phase 1B.

## Tooling & Version Control

This repo expects **Git LFS** for large binary assets.

Suggested setup:

```bash
git lfs install
git lfs pull
```

- Phase 1A includes UE target files required for project generation/build.

## UE 5.7 Troubleshooting

For Unreal Engine 5.7, target files use `BuildSettingsVersion.V6` and `EngineIncludeOrderVersion.Unreal5_7`.
