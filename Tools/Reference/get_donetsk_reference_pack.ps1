param(
    [string]$OutputDirectory = "",
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $RepoRoot 'ExternalAssets\DonetskReferences'
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

# Public civilian/historical architecture references only. Files stay under ExternalAssets/ (git-ignored).
# Special:Redirect/file resolves the current original media without hard-coding upload.wikimedia URLs.
$References = @(
    @{
        Id = 'ARTEMA60_FRONT'
        FileName = 'Donetsk artema 60.jpg'
        Page = 'https://commons.wikimedia.org/wiki/File:Donetsk_artema_60.jpg'
        Author = 'Andrew Butko'
        License = 'CC BY-SA 3.0 / GFDL (see source page)'
        Purpose = 'Artema 60 street facade, floor/window/projection study'
    },
    @{
        Id = 'ARTEMA60_OBLIQUE'
        FileName = 'Donetsk artema 60 2.jpg'
        Page = 'https://commons.wikimedia.org/wiki/File:Donetsk_artema_60_2.jpg'
        Author = 'Andrew Butko'
        License = 'CC BY-SA / GFDL (see source page)'
        Purpose = 'Artema 60 oblique facade proportions'
    },
    @{
        Id = 'ARTEMA60_HISTORIC'
        FileName = '1930. Donetsk artema 60.jpg'
        Page = 'https://commons.wikimedia.org/wiki/File:1930._Donetsk_artema_60.jpg'
        Author = 'Unknown / historical archive'
        License = 'Public-domain status recorded on source page; verify before redistribution'
        Purpose = 'Original pre-reconstruction constructivist massing'
    },
    @{
        Id = 'ARTEMA_STREET_2012'
        FileName = 'Artema - panoramio.jpg'
        Page = 'https://commons.wikimedia.org/wiki/File:Artema_-_panoramio.jpg'
        Author = 'Valery Ded'
        License = 'CC BY 3.0'
        Purpose = '2012 central Donetsk street wall, sidewalks and urban scale'
    },
    @{
        Id = 'STATION_2012_EXTERIOR'
        FileName = 'Donetsk rail station 2012.JPG'
        Page = 'https://commons.wikimedia.org/wiki/File:Donetsk_rail_station_2012.JPG'
        Author = 'MOs810'
        License = 'CC BY-SA 3.0'
        Purpose = 'Railway station exterior after 2012 reconstruction'
    },
    @{
        Id = 'STATION_2012_INTERIOR'
        FileName = 'Another angle inside donetsk voksal 2012.jpg'
        Page = 'https://commons.wikimedia.org/wiki/File:Another_angle_inside_donetsk_voksal_2012.jpg'
        Author = 'Annaslatt'
        License = 'CC BY-SA 4.0'
        Purpose = '2012 station interior/concourse material and structural language'
    },
    @{
        Id = 'ILICHA_HIGHRISE'
        FileName = 'Просп. Ілліча, 20а і 20.jpg'
        Page = 'https://commons.wikimedia.org/wiki/File:%D0%9F%D1%80%D0%BE%D1%81%D0%BF._%D0%86%D0%BB%D0%BB%D1%96%D1%87%D0%B0,_20%D0%B0_%D1%96_20.jpg'
        Author = 'See Wikimedia source page'
        License = 'See Wikimedia source page before redistribution'
        Purpose = 'Donetsk-specific high-rise residential morphology'
    },
    @{
        Id = 'ILICHA_BLOCKS'
        FileName = 'Просп. Ілліча, 24 і 26.jpg'
        Page = 'https://commons.wikimedia.org/wiki/File:%D0%9F%D1%80%D0%BE%D1%81%D0%BF._%D0%86%D0%BB%D0%BB%D1%96%D1%87%D0%B0,_24_%D1%96_26.jpg'
        Author = 'See Wikimedia source page'
        License = 'See Wikimedia source page before redistribution'
        Purpose = 'Donetsk-specific residential block/end-wall reference'
    }
)

$Attribution = @()
$Attribution += '# Donetsk Architecture Reference Pack'
$Attribution += ''
$Attribution += 'Downloaded for local art/reference use. Do not commit these media files to the repository.'
$Attribution += 'Always inspect each Wikimedia source page and comply with its current license before redistributing or deriving texture assets.'
$Attribution += ''

foreach ($Ref in $References) {
    $EncodedName = [System.Uri]::EscapeDataString($Ref.FileName)
    $DownloadUrl = "https://commons.wikimedia.org/wiki/Special:Redirect/file/$EncodedName"
    $Extension = [System.IO.Path]::GetExtension($Ref.FileName)
    $SafeId = $Ref.Id
    $Destination = Join-Path $OutputDirectory ("$SafeId$Extension")

    if ((Test-Path $Destination) -and -not $Force) {
        Write-Host "[TheUnit] Exists: $Destination"
    } else {
        Write-Host "[TheUnit] Downloading $($Ref.Id) ..."
        Invoke-WebRequest -Uri $DownloadUrl -OutFile $Destination -MaximumRedirection 8
    }

    $Attribution += "## $($Ref.Id)"
    $Attribution += "- Local file: $([System.IO.Path]::GetFileName($Destination))"
    $Attribution += "- Purpose: $($Ref.Purpose)"
    $Attribution += "- Source: $($Ref.Page)"
    $Attribution += "- Author: $($Ref.Author)"
    $Attribution += "- License note: $($Ref.License)"
    $Attribution += ''
}

$CategoryReferences = @(
    'https://commons.wikimedia.org/wiki/Category:Architecture_of_Donetsk',
    'https://commons.wikimedia.org/wiki/Category:Buildings_in_Donetsk_by_address',
    'https://commons.wikimedia.org/wiki/Category:Artema_Street,_Donetsk',
    'https://commons.wikimedia.org/wiki/Category:Artema_Street_76,_Donetsk',
    'https://commons.wikimedia.org/wiki/Category:Ilicha_Avenue,_Donetsk_(part_in_Kalininskyi_Raion)',
    'https://commons.wikimedia.org/wiki/Category:Rozy_Luxemburg_Street,_Donetsk',
    'https://commons.wikimedia.org/wiki/Category:Donetsk_Railway_Station'
)

$Attribution += '## Additional multi-angle categories'
foreach ($Url in $CategoryReferences) {
    $Attribution += "- $Url"
}

$AttributionPath = Join-Path $OutputDirectory 'ATTRIBUTION.md'
$Attribution -join "`r`n" | Set-Content -Path $AttributionPath -Encoding UTF8

$ArtemaFetcher = Join-Path $PSScriptRoot 'get_artema60_reference_set.ps1'
if (Test-Path $ArtemaFetcher) {
    Write-Host '[TheUnit] Fetching full Artema 60 building category for production calibration...'
    & $ArtemaFetcher -OutputDirectory (Join-Path $OutputDirectory 'Artema60_FullCategory') -Force:$Force
}

Write-Host "[TheUnit] Reference pack ready: $OutputDirectory"
Write-Host "[TheUnit] Attribution ledger: $AttributionPath"
