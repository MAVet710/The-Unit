$ErrorActionPreference='Stop'
$env:COMSPEC='C:\\Windows\\System32\\cmd.exe'
$repo='C:\Users\ndasi\OneDrive\Documents\GitHub\The-Unit'
$proj=Join-Path $repo 'TheUnit.uproject'
$ue='E:\UE_5.7'
$dotnet='E:\UE_5.7\Engine\Binaries\ThirdParty\DotNet\8.0.412\win-x64\dotnet.exe'
$ubt='E:\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
$report='E:\TheUnitWork\beta-entry-20260927\evidence\donetsk-production-v3-regression'
$release='E:\TheUnitWork\main-release-donetsk-production-v3-20260929'
$runlog='E:\TheUnitWork\donetsk-production-v3-runtime.log'
$summary='E:\TheUnitWork\donetsk-production-v3-summary.txt'
$screenshot='E:\TheUnitWork\donetsk-production-v3-runtime.png'
Remove-Item $summary -Force -ErrorAction SilentlyContinue
function Note([string]$s){ ((Get-Date).ToString('s')+' '+$s) | Tee-Object -FilePath $summary -Append }

Set-Location $repo
Note ('START HEAD='+(git rev-parse HEAD))

Note 'BUILD_GAME_START'
& $dotnet $ubt TheUnit Win64 Development $proj -WaitMutex -NoHotReloadFromIDE -NoXGE -NoUBA -MaxParallelActions=1 *>> $summary
if($LASTEXITCODE -ne 0){ throw "Game build failed $LASTEXITCODE" }
Note 'BUILD_GAME_OK'

Note 'TEST_START'
Remove-Item $report -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $report | Out-Null
& "$ue\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $proj -unattended -nop4 -nosplash -NullRHI '-ExecCmds=Automation RunTests TheUnit;Quit' "-ReportExportPath=$report" -TestExit='Automation Test Queue Empty' *>> $summary
$j=Get-Content "$report\index.json" -Raw|ConvertFrom-Json
Note "TEST_RESULT PASS=$($j.succeeded) FAIL=$($j.failed) WARN=$($j.succeededWithWarnings) TOTAL=$($j.tests.Count)"
if($j.failed -gt 0){ throw 'Regression failures' }

Note 'PACKAGE_START'
Remove-Item $release -Recurse -Force -ErrorAction SilentlyContinue
& "$ue\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="$proj" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="$release" -map='/Game/TheUnit/Maps/Donetsk' -utf8output *>> $summary
if($LASTEXITCODE -ne 0){ throw "Packaging failed $LASTEXITCODE" }
Note 'PACKAGE_OK'

$root=Join-Path $release 'Windows'
$exe=Join-Path $root 'TheUnit.exe'
if(!(Test-Path $exe)){ throw "Missing packaged exe $exe" }
Remove-Item $runlog,$screenshot -Force -ErrorAction SilentlyContinue
$p=Start-Process $exe -WorkingDirectory $root -ArgumentList @('/Game/TheUnit/Maps/Donetsk','-game','-windowed','-ResX=1280','-ResY=720','-NoSplash',"-abslog=$runlog") -PassThru
Start-Sleep 12

# Capture the actual packaged player viewport as release evidence.
try {
  Add-Type -AssemblyName System.Drawing
  if(-not ('TUWindowCapture' -as [type])){
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public struct TURECT { public int Left; public int Top; public int Right; public int Bottom; }
public static class TUWindowCapture {
  [DllImport("user32.dll")]
  public static extern bool GetWindowRect(IntPtr hWnd, out TURECT rect);
}
'@
  }
  $live=Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if($live -and $live.MainWindowHandle -ne 0){
    $rect=New-Object TURECT
    if([TUWindowCapture]::GetWindowRect($live.MainWindowHandle,[ref]$rect)){
      $w=$rect.Right-$rect.Left; $h=$rect.Bottom-$rect.Top
      if($w -gt 0 -and $h -gt 0){
        $bmp=New-Object System.Drawing.Bitmap $w,$h
        $gfx=[System.Drawing.Graphics]::FromImage($bmp)
        $gfx.CopyFromScreen($rect.Left,$rect.Top,0,0,$bmp.Size)
        $bmp.Save($screenshot,[System.Drawing.Imaging.ImageFormat]::Png)
        $gfx.Dispose(); $bmp.Dispose()
        Note "SCREENSHOT=$screenshot"
      }
    }
  }
} catch {
  Note ("SCREENSHOT_WARN="+$_.Exception.Message)
}

Start-Sleep 6
$alive=[bool](Get-Process -Id $p.Id -ErrorAction SilentlyContinue)
Note "RUNTIME_ALIVE=$alive"
if($alive){ Stop-Process -Id $p.Id -Force }
if(-not $alive){ throw 'Packaged Donetsk process exited before the runtime gate completed' }

if(Test-Path $runlog){
  $evidence=Select-String -Path $runlog -Pattern 'LoadMap:|DonetskMissionGameMode|Fatal error|Failed to load|Failed to find|Nanite'
  $evidence | Select-Object -Last 120 | ForEach-Object {$_.Line} | Out-File $summary -Append
  $loaded=[bool](Select-String -Path $runlog -Pattern 'LoadMap:.*Donetsk' -Quiet)
  Note "RUNTIME_DONETSK_LOADED=$loaded"
  if(-not $loaded){ throw 'Packaged runtime did not confirm the Donetsk map load' }

  $renderErrors=@(Select-String -Path $runlog -Pattern 'Fatal error|Failed to load|Failed to find|missing bUsedWithNanite|Default Material will be used in game|Unable to build Nanite|Failed to build Nanite')
  Note "RUNTIME_RENDER_ERRORS=$($renderErrors.Count)"
  if($renderErrors.Count -gt 0){
    $renderErrors | Select-Object -Last 80 | ForEach-Object {$_.Line} | Out-File $summary -Append
    throw "Packaged Donetsk runtime reported $($renderErrors.Count) fatal/load/Nanite rendering errors"
  }
} else {
  throw "Missing packaged runtime log $runlog"
}

Note ('FINAL_HEAD='+(git rev-parse HEAD))
Note ('RELEASE='+$release)
Note 'DONE'
