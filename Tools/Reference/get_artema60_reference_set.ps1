param(
    [string]$OutputDirectory = "",
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $RepoRoot 'ExternalAssets\DonetskReferences\Artema60_FullCategory'
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$Api = 'https://commons.wikimedia.org/w/api.php'
$CategoryTitle = 'Category:60 Artema Street, Donetsk'

function Get-SafeFileName([string]$Name) {
    $Invalid = [System.IO.Path]::GetInvalidFileNameChars()
    foreach ($Char in $Invalid) {
        $Name = $Name.Replace($Char, '_')
    }
    return $Name
}

function Get-FileInfo([string]$Title) {
    $EncodedTitle = [System.Uri]::EscapeDataString($Title)
    $Url = "$Api?action=query&format=json&prop=imageinfo&iiprop=url%7Csize%7Cextmetadata&titles=$EncodedTitle"
    $Response = Invoke-RestMethod -Uri $Url -Method Get
    $Page = $Response.query.pages.PSObject.Properties.Value | Select-Object -First 1
    if (-not $Page -or -not $Page.imageinfo) {
        throw "Wikimedia returned no image info for '$Title'."
    }
    return $Page.imageinfo[0]
}

$CategoryUrl = "$Api?action=query&format=json&list=categorymembers&cmtype=file&cmlimit=max&cmtitle=$([System.Uri]::EscapeDataString($CategoryTitle))"
$CategoryResponse = Invoke-RestMethod -Uri $CategoryUrl -Method Get
$Members = @($CategoryResponse.query.categorymembers)
if ($Members.Count -eq 0) {
    throw "No files were returned for $CategoryTitle"
}

$Ledger = @()
$Index = 0
foreach ($Member in $Members) {
    $Index++
    $Title = [string]$Member.title
    $OriginalName = $Title -replace '^File:', ''
    $Info = Get-FileInfo $Title
    $Extension = [System.IO.Path]::GetExtension($OriginalName)
    if ([string]::IsNullOrWhiteSpace($Extension)) {
        $Extension = '.jpg'
    }

    $SafeBase = Get-SafeFileName ([System.IO.Path]::GetFileNameWithoutExtension($OriginalName))
    $DestinationName = ('{0:D2}_{1}{2}' -f $Index, $SafeBase, $Extension)
    $Destination = Join-Path $OutputDirectory $DestinationName

    if ((Test-Path $Destination) -and -not $Force) {
        Write-Host "[TheUnit] Exists: $DestinationName"
    } else {
        Write-Host "[TheUnit] Downloading $Index/$($Members.Count): $OriginalName"
        Invoke-WebRequest -Uri $Info.url -OutFile $Destination -MaximumRedirection 8
    }

    $Meta = $Info.extmetadata
    $Ledger += [ordered]@{
        index = $Index
        title = $Title
        original_name = $OriginalName
        local_file = $DestinationName
        source_page = "https://commons.wikimedia.org/wiki/$([System.Uri]::EscapeDataString($Title.Replace(' ', '_')))"
        original_url = $Info.url
        width_px = $Info.width
        height_px = $Info.height
        author = if ($Meta.Artist) { $Meta.Artist.value } else { $null }
        license_short = if ($Meta.LicenseShortName) { $Meta.LicenseShortName.value } else { $null }
        license_url = if ($Meta.LicenseUrl) { $Meta.LicenseUrl.value } else { $null }
        credit = if ($Meta.Credit) { $Meta.Credit.value } else { $null }
        description = if ($Meta.ImageDescription) { $Meta.ImageDescription.value } else { $null }
    }
}

$LedgerPath = Join-Path $OutputDirectory 'artema60_reference_ledger.json'
$Ledger | ConvertTo-Json -Depth 8 | Set-Content -Path $LedgerPath -Encoding UTF8

$Readme = @(
    '# Artema 60 Full Wikimedia Reference Set',
    '',
    "Category: $CategoryTitle",
    "Files downloaded: $($Members.Count)",
    '',
    'These images are for local architecture/photo-match reference. They are intentionally stored under ExternalAssets/ and are not committed to Git.',
    'Each image retains its source/license metadata in artema60_reference_ledger.json. Review the source license before redistributing media or derived texture content.',
    '',
    'Photo-match order:',
    '1. establish current 4-storey street facade silhouette and total bay rhythm;',
    '2. calibrate semicircular projection radius/depth from oblique views;',
    '3. calibrate stair-tower projection and tall-window proportions;',
    '4. validate parapet/balustrade/column spacing;',
    '5. compare current mass against the 1930 three-storey historical image.'
)
$Readme -join "`r`n" | Set-Content -Path (Join-Path $OutputDirectory 'README.md') -Encoding UTF8

Write-Host "[TheUnit] Artema 60 full reference set ready: $OutputDirectory"
Write-Host "[TheUnit] Files: $($Members.Count)"
Write-Host "[TheUnit] Ledger: $LedgerPath"
