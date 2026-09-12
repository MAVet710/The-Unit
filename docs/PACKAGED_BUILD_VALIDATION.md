# Packaged build validation

**Result: BLOCKED / NOT EXECUTED. No packaged executable was produced or launched.**

## Evidence scope

- Source inspected: `1c39edbf916e4c6d7ddcf8ccb6c6cdfd8fee4ab6`, branch `agent/packaging`, based on the working-game integration effort.
- Inspection host: Linux x86_64, kernel `6.18.35`.
- Project declares UE `5.7`; game and editor targets declare `BuildSettingsVersion.V6` and `EngineIncludeOrderVersion.Unreal5_7`.
- `command -v` returned no executable for `UnrealEditor`, `UnrealEditor-Cmd`, `RunUAT.bat`, `Build.bat`, `pwsh`, `powershell`, `wine`, `cl`, or `dotnet`.
- File search under `/opt`, `/usr/local`, and `/workspace` found no `UnrealEditor*`, `RunUAT*`, `Build.bat`, or `TheUnit.exe` files. This is evidence about this host, not other machines.
- No `.github` workflow directory was present in the inspected snapshot. No Windows runner was exercised.

## Gates

| Gate | Result | Evidence |
| --- | --- | --- |
| TheUnitEditor Win64 Development | NOT RUN | Windows UE toolchain unavailable |
| Generate and save all three maps | NOT RUN | Requires built module and Unreal Editor |
| Development build/cook/stage/archive | NOT RUN | UAT unavailable |
| Development executable launch | NOT RUN | No artifact |
| Shipping build/cook/stage/archive | NOT RUN | UAT unavailable; Development gate unmet |
| Shipping executable launch | NOT RUN | No artifact |
| Packaged 42-step playthrough | NOT RUN | No running packaged game |
| FPS/thread/GPU performance | NOT MEASURED | No runtime session |

## Packaging configuration inspection

At the inspected commit, `Content/TheUnit/Maps` contained no `.umap` files. `Config/DefaultEngine.ini` was absent and no `MapsToCook` configuration existed. The generator `Tools/Unreal/create_end_to_end_maps.py` specifies these map packages and game modes:

| Map | Game mode |
| --- | --- |
| `/Game/TheUnit/Maps/CommandCenter` | `TU_HideoutGameMode` |
| `/Game/TheUnit/Maps/Killhouse` | `TU_TrainingMissionGameMode` |
| `/Game/TheUnit/Maps/Donetsk` | `TU_DonetskMissionGameMode` |

The generator writes CommandCenter startup/default-map settings only when it runs. A generator in source is not a generated or cooked map. Configuration changes integrated subsequently must be checked at the actual packaged commit.

`Scripts/ValidateTheUnit.ps1` builds the editor and invokes `TheUnit.` automation using `-NullRHI`; it does not package or launch the standalone game. Headless automation cannot validate rendered gameplay or GPU timing.

## Windows execution sequence (not executed here)

Run on a Windows host with UE 5.7 and its supported C++/Windows SDK toolchain already installed. These commands use repository scripts and local UAT; they introduce no service or dependency. Paths below resolve from the checkout, and archives remain outside the source repository. Stop on every failed stage. Capture each command's complete output and exit code in the validation record.

```powershell
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Get-Location).Path
$ProjectFile = Join-Path $ProjectRoot 'TheUnit.uproject'
$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
$UAT = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$ArchiveRoot = Join-Path (Split-Path $ProjectRoot -Parent) 'TheUnit-WindowsBuilds'

& .\Scripts\ValidateTheUnit.ps1 -EngineRoot $EngineRoot -ProjectPath $ProjectFile
& .\Tools\Unreal\create_end_to_end_maps.ps1 -UnrealEditorCmd $EditorCmd

foreach ($Map in @('CommandCenter', 'Killhouse', 'Donetsk')) {
    if (-not (Test-Path (Join-Path $ProjectRoot "Content\TheUnit\Maps\$Map.umap"))) {
        throw "Required generated map missing: $Map"
    }
}

$DevelopmentArchive = Join-Path $ArchiveRoot 'WindowsDevelopment'
& $UAT BuildCookRun "-project=$ProjectFile" -noP4 -platform=Win64 `
    -clientconfig=Development -build -cook -stage -pak -archive `
    '-map=/Game/TheUnit/Maps/CommandCenter+/Game/TheUnit/Maps/Killhouse+/Game/TheUnit/Maps/Donetsk' `
    "-archivedirectory=$DevelopmentArchive"
if ($LASTEXITCODE -ne 0) { throw "Development packaging failed: $LASTEXITCODE" }

$DevelopmentExe = Join-Path $DevelopmentArchive 'Windows\TheUnit.exe'
if (-not (Test-Path $DevelopmentExe)) {
    throw "Expected executable missing; inspect UAT archive output: $DevelopmentExe"
}
Start-Process -FilePath $DevelopmentExe -ArgumentList '-log' -PassThru
```

The executable path above is an expected UAT layout, **not an observed artifact path**. Inspect and record the actual archive before launch. Process creation alone is not a successful startup or playthrough. Confirm CommandCenter appears without the Editor, execute all 42 requested gameplay steps, and preserve logs/screenshots plus the exact commit and executable path. Verify three-map inclusion in cook output and travel in the packaged game. Only after Development validation succeeds:

```powershell
$ShippingArchive = Join-Path $ArchiveRoot 'WindowsShipping'
& $UAT BuildCookRun "-project=$ProjectFile" -noP4 -platform=Win64 `
    -clientconfig=Shipping -build -cook -stage -pak -archive `
    '-map=/Game/TheUnit/Maps/CommandCenter+/Game/TheUnit/Maps/Killhouse+/Game/TheUnit/Maps/Donetsk' `
    "-archivedirectory=$ShippingArchive"
if ($LASTEXITCODE -ne 0) { throw "Shipping packaging failed: $LASTEXITCODE" }

$ShippingExe = Join-Path $ShippingArchive 'Windows\TheUnit.exe'
if (-not (Test-Path $ShippingExe)) {
    throw "Expected executable missing; inspect UAT archive output: $ShippingExe"
}
Start-Process -FilePath $ShippingExe -PassThru
```

Verify visible Shipping startup, playable controls, mission/extraction travel, and restart persistence. Shipping logs may be disabled, so retain independent runtime evidence. Keep generated `.umap` assets in source control after inspection; do not commit archive output, `Saved`, `Intermediate`, `Binaries`, or `DerivedDataCache`.

## Blockers

### Windows UE execution unavailable

- **BLOCKER:** No executable Windows UE 5.7 build/runtime environment in this workspace.
- **SYSTEM:** Build, cook, package, launch, performance, end-to-end validation.
- **ERROR:** Tool lookup returned no paths; filesystem search returned no matching tool binaries. No UAT/compiler error is claimed because neither ran.
- **LOG:** Shell environment/tool discovery during packaging audit; no UE/UAT runtime log exists.
- **ROOT CAUSE:** Available execution host is Linux and the inspected paths contain no Unreal installation or Windows compiler/runtime.
- **FIX:** No environment substitution or fabricated artifacts. Execute the integration branch on an available Windows UE 5.7 host using the local command sequence above.
- **RETEST:** NOT RUN.

### Serialized maps absent at inspected source commit

- **BLOCKER:** Required runtime map assets have not been generated in the inspected snapshot.
- **SYSTEM:** Map bootstrap and cook.
- **ERROR:** Repository file inspection found no `.umap` assets.
- **LOG:** `Tools/Unreal/create_end_to_end_maps.py` contains generator definitions; no map-generation run log exists.
- **ROOT CAUSE:** Editor asset generation has not been demonstrated in this environment.
- **FIX:** Maps workstream owns configuration/generator fixes. After editor compilation, execute the generator, inspect all maps in Unreal, and commit its saved assets.
- **RETEST:** NOT RUN. A packaging command with explicit map names cannot substitute for missing assets.

## Artifact record

- Integrated commit actually packaged: none.
- Compiler version used: none.
- Development archive/executable: none.
- Shipping archive/executable: none.
- Executable startup evidence: none.
- Completed packaged gameplay steps: 0/42.
