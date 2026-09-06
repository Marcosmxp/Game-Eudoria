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
    Write-Host "[Role payload] Python not found; exact symbol1998 diagnostic/auto layer skipped."
    exit 0
}

$root = (Resolve-Path ".").Path
$outputPath = Join-Path $root $Output
$outputDirectory = Split-Path $outputPath -Parent
$autoDirectory = Join-Path $outputDirectory "auto"
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-role-payload-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $outputDirectory, $autoDirectory, $tempPath | Out-Null

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

function Copy-AutoVisual {
    param(
        [Parameter(Mandatory = $true)][PSCustomObject]$Row,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $characterId = [int]$Row.characterId
    $frame = [string]$Row.sourceFrame
    $type = [string]$Row.characterType
    $entry = Resolve-ArchiveEntry -Type $type -CharacterId $characterId -Frame $frame

    if (-not $entry) {
        Write-Host "[Role payload] Asset path not found for character $characterId ($type); skipping $($Row.name)."
        return $false
    }

    if (-not (Extract-ResolvedEntry -Entry $entry -Destination $Destination)) {
        Write-Host "[Role payload] Failed to extract $entry for $($Row.name)."
        return $false
    }
    return $true
}

try {
    & $SevenZip x $Archive "scripts/_assets/assets.swf" "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Could not extract assets.swf from Crystal Saga.rar" }

    $swf = Join-Path $tempPath "scripts\_assets\assets.swf"
    if (-not (Test-Path $swf)) { throw "Extracted assets.swf was not found: $swf" }

    $tool = Join-Path $root "tools\swf_ui_payload\swf_ui_payload.py"
    if (-not (Test-Path $tool)) { throw "SWF payload parser was not found: $tool" }

    $exitCode = Invoke-PythonScript -Arguments @($tool, $swf, "--output", $outputPath, "--exports", "symbol1998")
    if ($exitCode -ne 0) {
        Write-Host "[Role payload] Exact symbol1998 parser is not required for runtime; diagnostic dump skipped."
        exit 0
    }

    $payload = Get-Content $outputPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $component = $payload.components | Select-Object -First 1
    $tsv = Join-Path $outputDirectory "payload.tsv"

    @(
        "depth`tname`tcharacterId`tcharacterType`tx`ty`tscaleX`tscaleY`tvisible"
        foreach ($child in $component.children) {
            $name = if ($null -ne $child.name) { [string]$child.name } else { "" }
            $characterId = if ($null -ne $child.characterId) { [string]$child.characterId } else { "" }
            $type = if ($null -ne $child.characterType) { [string]$child.characterType } else { "" }
            $x = if ($null -ne $child.transform) { [string]$child.transform.x } else { "0" }
            $y = if ($null -ne $child.transform) { [string]$child.transform.y } else { "0" }
            $sx = if ($null -ne $child.transform) { [string]$child.transform.scaleX } else { "1" }
            $sy = if ($null -ne $child.transform) { [string]$child.transform.scaleY } else { "1" }
            $visible = if ($null -ne $child.visible) { [string]$child.visible } else { "" }
            "$($child.depth)`t$name`t$characterId`t$type`t$x`t$y`t$sx`t$sy`t$visible"
        }
    ) | Set-Content -Path $tsv -Encoding UTF8

    Write-Host "Role Character exact symbol1998 payload written to $outputPath"
    Write-Host "Role Character display-list table written to $tsv"

    $manifestTool = Join-Path $root "tools\swf_ui_payload\role_character_manifest.py"
    $manifest = Join-Path $outputDirectory "auto_manifest.tsv"
    $rootBounds = Join-Path $outputDirectory "reference_bounds.tsv"
    if (Test-Path $manifestTool) {
        $exitCode = Invoke-PythonScript -Arguments @(
            $manifestTool,
            $swf,
            "--output", $manifest,
            "--root-bounds-output", $rootBounds,
            "--exclude-ids", "1884", "276", "304", "263", "361", "89", "908", "1914", "1917"
        )
        if ($exitCode -eq 0 -and (Test-Path $manifest)) {
            Remove-Item $autoDirectory -Recurse -Force -ErrorAction SilentlyContinue
            New-Item -ItemType Directory -Force -Path $autoDirectory | Out-Null

            $rows = Import-Csv -Path $manifest -Delimiter "`t"
            $copied = 0
            $failed = @()
            foreach ($row in $rows) {
                $destination = Join-Path $autoDirectory ([string]$row.asset)
                if (Copy-AutoVisual -Row $row -Destination $destination) { $copied++ }
                else { $failed += "$($row.depth):$($row.name):$($row.characterId):$($row.characterType)" }
            }

            $failedPath = Join-Path $outputDirectory "auto_missing.txt"
            if ($failed.Count -gt 0) { $failed | Set-Content -Path $failedPath -Encoding UTF8 }
            else { Remove-Item $failedPath -Force -ErrorAction SilentlyContinue }

            Write-Host "Role Character auto payload layer extracted: $copied / $($rows.Count) visuals."
            if (Test-Path $rootBounds) {
                Write-Host "Role Character reference bounds written to $rootBounds"
            }
            if ($failed.Count -gt 0) {
                Write-Host "[Role payload] Missing auto visuals recorded in $failedPath"
            }
        }
        else {
            Write-Host "[Role payload] Static visual manifest generation failed; keeping the already-written runtime reference bounds."
        }
    }
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}
