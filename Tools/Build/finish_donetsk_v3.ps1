$ErrorActionPreference='Stop'
$repo='C:\Users\ndasi\OneDrive\Documents\GitHub\The-Unit'
$proj=Join-Path $repo 'TheUnit.uproject'
$ue='E:\UE_5.7'
$dotnet='E:\UE_5.7\Engine\Binaries\ThirdParty\DotNet\8.0.412\win-x64\dotnet.exe'
$ubt='E:\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
$report='E:\TheUnitWork\beta-entry-20260927\evidence\donetsk-production-v3-regression'
$release='E:\TheUnitWork\main-release-donetsk-production-v3-20260929'
$runlog='E:\TheUnitWork\donetsk-production-v3-runtime.log'
$summary='E:\TheUnitWork\donetsk-production-v3-summary.txt'
Remove-Item $summary -Force -ErrorAction SilentlyContinue
function Note([string]$s){ ((Get-Date).ToString('s')+' '+$s) | Tee-Object -FilePath $summary -Append }

Set-Location $repo
Note ('START HEAD='+(git rev-parse HEAD))

Note 'BUILD_GAME_START'
& "$ue\Engine\Build\BatchFiles\Build.bat" TheUnit Win64 Development $proj -WaitMutex -NoHotReloadFromIDE -NoXGE -NoUBA -MaxParallelActions=1 *>> $summary
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
Remove-Item $runlog -Force -ErrorAction SilentlyContinue
$p=Start-Process $exe -WorkingDirectory $root -ArgumentList @('/Game/TheUnit/Maps/Donetsk','-game','-windowed','-ResX=1280','-ResY=720','-NoSplash',"-abslog=$runlog") -PassThru
Start-Sleep 18
$alive=[bool](Get-Process -Id $p.Id -ErrorAction SilentlyContinue)
Note "RUNTIME_ALIVE=$alive"
if($alive){ Stop-Process -Id $p.Id -Force }
if(Test-Path $runlog){
  Select-String -Path $runlog -Pattern 'LoadMap:|DonetskMissionGameMode|Fatal error|Failed to load|Failed to find|Nanite' |
    Select-Object -Last 80 | ForEach-Object {$_.Line} | Out-File $summary -Append
}

Note ('FINAL_HEAD='+(git rev-parse HEAD))
Note ('RELEASE='+$release)
Note 'DONE'
