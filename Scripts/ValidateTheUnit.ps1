[CmdletBinding()]
param(
    [string]$EngineRoot = "",
    [string]$ProjectPath = "",
    [string]$TestFilter = "TheUnit."
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $ProjectPath = Join-Path $RepoRoot "TheUnit.uproject"
}
$ProjectPath = (Resolve-Path $ProjectPath).Path

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    if (-not [string]::IsNullOrWhiteSpace($env:UE_ENGINE_ROOT)) {
        $EngineRoot = $env:UE_ENGINE_ROOT
    }
    else {
        $EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
    }
}

$BuildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

foreach ($RequiredPath in @($BuildBat, $EditorCmd, $ProjectPath)) {
    if (-not (Test-Path $RequiredPath)) {
        throw "Required path not found: $RequiredPath"
    }
}

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$Description
    )

    Write-Host "`n==> $Description"
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE."
    }
}

Invoke-CheckedNative `
    -FilePath $BuildBat `
    -Description "Build TheUnitEditor Win64 Development" `
    -Arguments @(
        "TheUnitEditor",
        "Win64",
        "Development",
        "-Project=$ProjectPath",
        "-WaitMutex",
        "-NoHotReloadFromIDE"
    )

Invoke-CheckedNative `
    -FilePath $EditorCmd `
    -Description "Run Unreal automation tests: $TestFilter" `
    -Arguments @(
        $ProjectPath,
        "/Engine/Maps/Entry",
        "-unattended",
        "-NullRHI",
        "-nosound",
        "-ExecCmds=Automation RunTests $TestFilter",
        "-TestExit=Automation Test Queue Empty"
    )

Write-Host "`nThe Unit validation completed successfully."
