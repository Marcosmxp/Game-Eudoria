param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [string]$Output = "legacy_assets/runtime/ui/role_window/character/payload.json",
    [string]$SevenZip = "7z"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $Archive)) {
    throw "Archive not found: $Archive"
}
if (-not (Get-Command $SevenZip -ErrorAction SilentlyContinue)) {
    throw "7-Zip was not found."
}

$python = $null
foreach ($candidate in @("python", "python3", "py")) {
    if (Get-Command $candidate -ErrorAction SilentlyContinue) {
        $python = $candidate
        break
    }
}
if (-not $python) {
    throw "Python is required to prepare the exact Character UI manifests."
}

$root = (Resolve-Path ".").Path
$outputPath = Join-Path $root $Output
$outputDirectory = Split-Path $outputPath -Parent
$visualDirectory = Join-Path $outputDirectory "visual"
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-role-payload-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $outputDirectory, $visualDirectory, $tempPath | Out-Null

function Invoke-PythonScript {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)
    if ($python -eq "py") {
        & $python -3 @Arguments
    }
    else {
        & $python @Arguments
    }
    return $LASTEXITCODE
}

$archiveEntries = @()
$listing = & $SevenZip l -slt $Archive
if ($LASTEXITCODE -ne 0) {
    throw "Could not list Crystal Saga.rar"
}
foreach ($line in $listing) {
    if ($line -like "Path = *") {
        $value = $line.Substring(7).Replace('\', '/')
        if ($value -and $value -ne ($Archive.Replace('\', '/'))) {
            $archiveEntries += $value
        }
    }
}

function Resolve-ArchiveEntry {
    param(
        [Parameter(Mandatory = $true)][string]$Type,
        [Parameter(Mandatory = $true)][int]$CharacterId,
        [Parameter(Mandatory = $true)][string]$Frame
    )

    $escapedId = [regex]::Escape([string]$CharacterId)
    switch ($Type) {
        { $_ -like "shape*" } { $pattern = "^shapes/$escapedId\.png$" }
        "sprite" {
            $escapedFrame = [regex]::Escape($Frame)
            $pattern = "^sprites/DefineSprite_${escapedId}(?:_[^/]*)?/${escapedFrame}\.png$"
        }
        "button" { $pattern = "^buttons/DefineButton_${escapedId}(?:_[^/]*)?/1_up\.png$" }
        "button2" { $pattern = "^buttons/DefineButton2_${escapedId}(?:_[^/]*)?/1_up\.png$" }
        default { return $null }
    }
    return $archiveEntries | Where-Object { $_ -match $pattern } | Select-Object -First 1
}

function Extract-ResolvedEntry {
    param(
        [Parameter(Mandatory = $true)][string]$Entry,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    & $SevenZip x $Archive $Entry "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) { return $false }

    $source = Join-Path $tempPath ($Entry -replace '/', [IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path $source)) { return $false }

    New-Item -ItemType Directory -Force -Path (Split-Path $Destination -Parent) | Out-Null
    Copy-Item $source $Destination -Force
    return $true
}

try {
    & $SevenZip x $Archive "scripts/_assets/assets.swf" "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Could not extract scripts/_assets/assets.swf from Crystal Saga.rar"
    }

    $swf = Join-Path $tempPath "scripts\_assets\assets.swf"
    if (-not (Test-Path $swf)) {
        throw "Extracted assets.swf was not found: $swf"
    }

    $manifestTool = Join-Path $root "tools\swf_ui_payload\role_character_manifest.py"
    if (-not (Test-Path $manifestTool)) {
        throw "Role Character manifest tool was not found: $manifestTool"
    }

    $visualManifest = Join-Path $outputDirectory "visual_manifest.tsv"
    $textManifest = Join-Path $outputDirectory "text_manifest.tsv"
    $rootBounds = Join-Path $outputDirectory "reference_bounds.tsv"

    $exitCode = Invoke-PythonScript -Arguments @(
        $manifestTool,
        $swf,
        "--output", $visualManifest,
        "--text-output", $textManifest,
        "--root-bounds-output", $rootBounds
    )
    if ($exitCode -ne 0) {
        throw "Exact symbol1998 Character manifest generation failed"
    }

    Remove-Item $visualDirectory -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $visualDirectory | Out-Null

    $rows = Import-Csv -Path $visualManifest -Delimiter "`t"
    $copied = 0
    $failed = @()
    foreach ($row in $rows) {
        $characterId = [int]$row.characterId
        $entry = Resolve-ArchiveEntry -Type ([string]$row.characterType) -CharacterId $characterId -Frame ([string]$row.sourceFrame)
        if (-not $entry) {
            $failed += "$($row.depth):$($row.name):${characterId}:$($row.characterType)"
            continue
        }

        $destination = Join-Path $visualDirectory ([string]$row.asset)
        if (Extract-ResolvedEntry -Entry $entry -Destination $destination) {
            $copied++
        }
        else {
            $failed += "$($row.depth):$($row.name):${characterId}:$($row.characterType)"
        }
    }

    $failedPath = Join-Path $outputDirectory "visual_missing.txt"
    if ($failed.Count -gt 0) {
        $failed | Set-Content -Path $failedPath -Encoding UTF8
    }
    else {
        Remove-Item $failedPath -Force -ErrorAction SilentlyContinue
    }

    Write-Host "Role Character exact visual manifest: $($rows.Count) display objects."
    Write-Host "Role Character exact text manifest generated from DefineEditText fields."
    Write-Host "Role Character visual assets extracted: $copied / $($rows.Count)."
    if ($failed.Count -gt 0) {
        Write-Host "[Role Character] Missing visual assets recorded in $failedPath"
    }
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}
