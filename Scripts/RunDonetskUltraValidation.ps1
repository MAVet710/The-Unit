param(
    [string]$EngineRoot = "",
    [string]$ProjectPath = "",
    [string]$LogPath = "",
    [int]$AutoExitSeconds = 0,
    [switch]$Automated
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
    elseif (Test-Path "E:\UE_5.7") {
        $EngineRoot = "E:\UE_5.7"
    }
    else {
        $EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
    }
}

$EditorBinary = if ($Automated) { "UnrealEditor-Cmd.exe" } else { "UnrealEditor.exe" }
$EditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\$EditorBinary"
if (-not (Test-Path $EditorExe)) {
    throw "Unreal Editor not found: $EditorExe"
}

if ([string]::IsNullOrWhiteSpace($LogPath)) {
    $EvidenceDir = Join-Path $RepoRoot "Saved\Automation\DonetskVisual"
    New-Item -ItemType Directory -Force $EvidenceDir | Out-Null
    $Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $LogPath = Join-Path $EvidenceDir "DonetskUltra-$Stamp.log"
}

# Keep every Ultra override visible and auditable here. Normal user settings stay
# unchanged, and hardware Lumen is requested only by this validation path.
$UltraCVars = @(
    "sg.ResolutionQuality=100",
    "sg.ViewDistanceQuality=3",
    "sg.AntiAliasingQuality=3",
    "sg.ShadowQuality=3",
    "sg.TextureQuality=3",
    "sg.EffectsQuality=3",
    "sg.PostProcessQuality=3",
    "sg.GlobalIlluminationQuality=3",
    "sg.ReflectionQuality=3",
    "sg.ShadingQuality=3",
    "sg.FoliageQuality=3",
    "sg.LandscapeQuality=3",
    "r.AntiAliasingMethod=4",
    "r.DynamicGlobalIlluminationMethod=1",
    "r.ReflectionMethod=1",
    "r.Shadow.Virtual.Enable=1",
    "r.Nanite=1",
    "r.Lumen.HardwareRayTracing=1"
)

$SetCommands = $UltraCVars | ForEach-Object { $_ -replace "=", " " }
$QueryCommands = @(
    "r.AntiAliasingMethod",
    "r.DynamicGlobalIlluminationMethod",
    "r.ReflectionMethod",
    "r.Shadow.Virtual.Enable",
    "r.Nanite",
    "r.Lumen.HardwareRayTracing"
)
$CommandList = $SetCommands + $QueryCommands
if ($Automated) {
    $CommandList += "quit"
}
$ExecCommands = ($CommandList -join ",")

$Arguments = @(
    $ProjectPath,
    "/Game/TheUnit/Maps/Donetsk",
    "-game",
    "-windowed",
    "-ResX=2560",
    "-ResY=1440",
    "-ForceRes",
    "-dx12",
    "-noxgeshadercompile",
    "-NoSplash",
    "-NoLoadingScreen",
    "-ExecCmds=$ExecCommands",
    "-abslog=$LogPath"
)

if (-not [string]::IsNullOrWhiteSpace($CaptureId)) {
    if ($CaptureId -notmatch '^[A-Za-z0-9_-]{1,96}$') {
        throw "CaptureId contains unsupported characters."
    }
    $Arguments += "-RenderOffscreen"
    $Arguments += "-TUDonetskVisual=$CaptureId"
}
if ($Automated) {
    $Arguments += @("-unattended", "-nosound", "-stdout", "-FullStdOutLogOutput")
}

Write-Host "Launching Donetsk Ultra validation."
Write-Host "Log: $LogPath"

if ($Automated) {
    & $EditorExe @Arguments
    exit $LASTEXITCODE
}

$Process = Start-Process $EditorExe -ArgumentList $Arguments -PassThru
Write-Host "PID: $($Process.Id)"

if ($AutoExitSeconds -gt 0) {
    Start-Sleep -Seconds $AutoExitSeconds
    $Live = Get-Process -Id $Process.Id -ErrorAction SilentlyContinue
    if ($Live) {
        Stop-Process -Id $Process.Id -Force
        Write-Host "Stopped validation process after $AutoExitSeconds seconds."
    }
}
else {
    Write-Host "Ultra validation is running. Close the game window when review is complete."
}
