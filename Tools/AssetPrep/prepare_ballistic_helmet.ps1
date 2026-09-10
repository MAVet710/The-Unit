param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDirectory,

    [string]$OutputDirectory,

    [switch]$Force
)

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $RepoRoot 'ExternalAssets\BallisticHelmet'
}

$SourceDirectory = (Resolve-Path $SourceDirectory).Path
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)

$ArchiveNames = @(
    'Example_FBX.rar',
    'Example_OBJ.rar',
    'Example_BLEND.rar',
    'Example_ABC.rar',
    'Example_STL.rar',
    'Texture.rar'
)

$ExpectedLeaves = @(
    'Example.fbx',
    'Example.obj',
    'Example.mtl',
    'Example.blend',
    'Example.abc',
    'Example.stl',
    '3_helmet_color.tga',
    '3_helmet_norm.tga',
    '3_helmet_exp.tga'
)

foreach ($ArchiveName in $ArchiveNames) {
    $ArchivePath = Join-Path $SourceDirectory $ArchiveName
    if (-not (Test-Path -LiteralPath $ArchivePath -PathType Leaf)) {
        throw "Missing required helmet archive: $ArchivePath"
    }
}

if (Test-Path -LiteralPath $OutputDirectory) {
    $Existing = @(Get-ChildItem -LiteralPath $OutputDirectory -Force -ErrorAction SilentlyContinue)
    if ($Existing.Count -gt 0 -and -not $Force) {
        throw "Output directory is not empty: $OutputDirectory`nRe-run with -Force only if you intend to replace the working intake."
    }
    if ($Force) {
        Remove-Item -LiteralPath $OutputDirectory -Recurse -Force
    }
}

$RawDirectory = Join-Path $OutputDirectory 'Raw'
$WorkingDirectory = Join-Path $OutputDirectory 'Working'
New-Item -ItemType Directory -Path $RawDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $WorkingDirectory -Force | Out-Null

$SevenZip = Get-Command '7z.exe' -ErrorAction SilentlyContinue
if (-not $SevenZip) { $SevenZip = Get-Command '7z' -ErrorAction SilentlyContinue }
$Tar = Get-Command 'tar.exe' -ErrorAction SilentlyContinue
if (-not $Tar) { $Tar = Get-Command 'tar' -ErrorAction SilentlyContinue }

function Expand-RarArchive {
    param(
        [Parameter(Mandatory = $true)][string]$ArchivePath,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null

    if ($SevenZip) {
        & $SevenZip.Source x '-y' "-o$Destination" $ArchivePath | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "7-Zip failed to extract $ArchivePath (exit $LASTEXITCODE)."
        }
        return
    }

    if ($Tar) {
        & $Tar.Source -xf $ArchivePath -C $Destination
        if ($LASTEXITCODE -eq 0) {
            return
        }
    }

    throw 'No working RAR extractor found. Install 7-Zip or provide a tar/libarchive build with RAR support.'
}

foreach ($ArchiveName in $ArchiveNames) {
    $ArchivePath = Join-Path $SourceDirectory $ArchiveName
    $ArchiveStem = [System.IO.Path]::GetFileNameWithoutExtension($ArchiveName)
    $Destination = Join-Path $RawDirectory $ArchiveStem
    Write-Host "Extracting $ArchiveName"
    Expand-RarArchive -ArchivePath $ArchivePath -Destination $Destination
}

$Resolved = @{}
foreach ($Leaf in $ExpectedLeaves) {
    $Matches = @(Get-ChildItem -LiteralPath $RawDirectory -Recurse -File | Where-Object { $_.Name -eq $Leaf })
    if ($Matches.Count -ne 1) {
        throw "Expected exactly one '$Leaf' after extraction; found $($Matches.Count)."
    }
    $Resolved[$Leaf] = $Matches[0]
}

$CanonicalCopies = @(
    'Example.fbx',
    'Example.blend',
    '3_helmet_color.tga',
    '3_helmet_norm.tga',
    '3_helmet_exp.tga'
)

foreach ($Leaf in $CanonicalCopies) {
    Copy-Item -LiteralPath $Resolved[$Leaf].FullName -Destination (Join-Path $WorkingDirectory $Leaf) -Force
}

$ManifestFiles = foreach ($Leaf in $ExpectedLeaves) {
    $File = $Resolved[$Leaf]
    $Hash = Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256
    [ordered]@{
        name = $Leaf
        source_path = $File.FullName
        size_bytes = $File.Length
        sha256 = $Hash.Hash.ToLowerInvariant()
        role = switch ($Leaf) {
            'Example.fbx' { 'preferred_unreal_static_mesh_import' }
            'Example.blend' { 'editable_master_source' }
            '3_helmet_color.tga' { 'base_color_texture' }
            '3_helmet_norm.tga' { 'normal_texture' }
            '3_helmet_exp.tga' { 'unknown_packed_or_surface_map_inspect_before_wiring' }
            default { 'reference_or_fallback_format' }
        }
    }
}

$Manifest = [ordered]@{
    asset_id = 'ballistic_helmet_user_supplied_2026_09_07'
    generated_utc = [DateTime]::UtcNow.ToString('o')
    source_directory = $SourceDirectory
    output_directory = $OutputDirectory
    preferred_unreal_mesh = (Join-Path $WorkingDirectory 'Example.fbx')
    editable_master = (Join-Path $WorkingDirectory 'Example.blend')
    intended_equipment_slot = 'Headwear'
    intended_runtime_asset = 'SM_BallisticHelmet'
    intended_definition_asset = 'EQ_BallisticHelmet'
    ballistic_rating = $null
    ballistic_rating_note = 'Do not assign NIJ or other protection values until the represented helmet/spec is identified.'
    files = $ManifestFiles
}

$ManifestPath = Join-Path $OutputDirectory 'helmet_manifest.json'
$Manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ManifestPath -Encoding UTF8

Write-Host ''
Write-Host 'Ballistic helmet intake complete.'
Write-Host "Working FBX:   $(Join-Path $WorkingDirectory 'Example.fbx')"
Write-Host "Working Blend: $(Join-Path $WorkingDirectory 'Example.blend')"
Write-Host "Manifest:      $ManifestPath"
Write-Host 'The ExternalAssets directory is ignored by Git; keep the licensed source binaries there.'
