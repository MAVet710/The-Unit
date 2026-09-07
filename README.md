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

## How to use TU_KillhouseGenerator

`TU_KillhouseGenerator` now builds an open-top modular shoot-house layout modeled after the supplied reference images instead of the older enclosed two-story graybox.

The generated structure includes:
- A large rectangular open-top training shell.
- Multiple asymmetric ground-floor rooms with working doorway gaps.
- Three larger rooms across the rear section.
- A broad central staircase.
- A full-width elevated observation catwalk.
- Guard rails, structural support beams, and wall-top posts.
- A right-side exterior return staircase and landing.
- Existing player-start, objective, extraction, patrol, and enemy-spawn markers.
- Optional editor-assigned wall, floor, and metal materials.

To use it:
1. Open a level in Unreal Editor.
2. Drag `TU_KillhouseGenerator` into the scene.
3. The actor auto-builds the kill house through `OnConstruction()`.
4. In Details, tune `BuildingWidth`, `BuildingLength`, wall/door dimensions, catwalk dimensions, rail dimensions, and stair dimensions.
5. Leave `Generate Roof` disabled for the intended open-top look.
6. Assign `Wall Material`, `Floor Material`, and `Metal Material` to replace the engine graybox appearance without changing the procedural layout.
7. Toggle `Generate Exterior Stair`, `Generate Wall Posts`, or `Generate Debug Labels` as needed.

The generator uses engine cube primitives, so the layout is immediately playable without importing a binary 3D asset. Final production meshes/materials can replace the graybox later while preserving the same spatial design.

## How to use TU_StadiumGenerator

1. Open a level in Unreal Editor.
2. Drag `TU_StadiumGenerator` into the scene.
3. Toggle procedural sections (field/walls/stands/shell/markers) in Details.
4. Adjust public-reference field dimensions and `FieldScale` as needed.
5. The actor auto-rebuilds the asymmetrical graybox stadium in-editor via `OnConstruction()`.
