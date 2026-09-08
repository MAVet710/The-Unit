param(
    [string]$UnrealEditorCmd = "",
    [switch]$KeepEditorOpen
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$Project = Join-Path $RepoRoot 'TheUnit.uproject'
$PythonScript = Join-Path $RepoRoot 'Tools\Unreal\create_end_to_end_maps.py'

if (-not (Test-Path $Project)) {
    throw "TheUnit.uproject was not found at $Project"
}
if (-not (Test-Path $PythonScript)) {
    throw "Map bootstrap script was not found at $PythonScript"
}

if ([string]::IsNullOrWhiteSpace($UnrealEditorCmd)) {
    $Candidates = @(
        'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
        'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe'
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            $UnrealEditorCmd = $Candidate
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace($UnrealEditorCmd) -or -not (Test-Path $UnrealEditorCmd)) {
    throw @"
Unreal Engine 5.7 was not found automatically.
Run again with the full editor path, for example:
  .\Tools\Unreal\create_end_to_end_maps.ps1 -UnrealEditorCmd 'D:\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
"@
}

Write-Host "[TheUnit] Project: $Project"
Write-Host "[TheUnit] Unreal:  $UnrealEditorCmd"
Write-Host "[TheUnit] Script:  $PythonScript"

$Arguments = @(
    $Project,
    '-nop4',
    '-nosplash',
    '-unattended',
    "-ExecutePythonScript=$PythonScript"
)

& $UnrealEditorCmd @Arguments
if ($LASTEXITCODE -ne 0) {
    throw "Unreal map bootstrap failed with exit code $LASTEXITCODE. Check Saved\Logs\TheUnit.log."
}

$CommandCenter = Join-Path $RepoRoot 'Content\TheUnit\Maps\CommandCenter.umap'
$Killhouse = Join-Path $RepoRoot 'Content\TheUnit\Maps\Killhouse.umap'
$Donetsk = Join-Path $RepoRoot 'Content\TheUnit\Maps\Donetsk.umap'

if (-not (Test-Path $CommandCenter) -or -not (Test-Path $Killhouse) -or -not (Test-Path $Donetsk)) {
    throw "Unreal exited successfully but one or more expected map files were not created. Check Saved\Logs\TheUnit.log."
}

Write-Host '[TheUnit] CommandCenter.umap created.'
Write-Host '[TheUnit] Killhouse.umap created.'
Write-Host '[TheUnit] Donetsk.umap created with the Donetsk reference-district GameMode.'
Write-Host '[TheUnit] CommandCenter configured as startup/default map.'

if ($KeepEditorOpen) {
    $EditorExe = $UnrealEditorCmd -replace 'UnrealEditor-Cmd\.exe$', 'UnrealEditor.exe'
    if (Test-Path $EditorExe) {
        Start-Process -FilePath $EditorExe -ArgumentList @($Project, '/Game/TheUnit/Maps/CommandCenter')
    }
}
