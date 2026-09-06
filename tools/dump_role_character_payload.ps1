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

function Copy-AutoVisual {
    param(
        [Parameter(Mandatory = $true)][PSCustomObject]$Row,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $characterId = [int]$Row.characterId
    $frame = [string]$Row.sourceFrame
    $type = [string]$Row.characterType
    $source = $null

    if ($type -like "shape*") {
        $entry = "shapes/$characterId.png"
        & $SevenZip x $Archive $entry "-o$tempPath" -y | Out-Null
        if ($LASTEXITCODE -eq 0) {
            $candidate = Join-Path $tempPath ($entry -replace '/', [IO.Path]::DirectorySeparatorChar)
            if (Test-Path $candidate) {
                $source = Get-Item $candidate
            }
        }
    }
    elseif ($type -eq "sprite") {
        $entry = "sprites/DefineSprite_$characterId*/$frame.png"
        & $SevenZip x $Archive $entry "-o$tempPath" -y | Out-Null
        if ($LASTEXITCODE -eq 0) {
            $spritesRoot = Join-Path $tempPath "sprites"
            if (Test-Path $spritesRoot) {
                $source = Get-ChildItem -Path $spritesRoot -Recurse -File -Filter "$frame.png" |
                    Where-Object { $_.Directory.Name -like "DefineSprite_$characterId*" } |
                    Select-Object -First 1
            }
        }
    }
    elseif ($type -eq "button" -or $type -eq "button2") {
        $prefix = if ($type -eq "button2") { "DefineButton2" } else { "DefineButton" }
        $entry = "buttons/${prefix}_$characterId*/1_up.png"
        & $SevenZip x $Archive $entry "-o$tempPath" -y | Out-Null
        if ($LASTEXITCODE -eq 0) {
            $buttonsRoot = Join-Path $tempPath "buttons"
            if (Test-Path $buttonsRoot) {
                $source = Get-ChildItem -Path $buttonsRoot -Recurse -File -Filter "1_up.png" |
                    Where-Object { $_.Directory.Name -like "${prefix}_$characterId*" } |
                    Select-Object -First 1
            }
        }
    }

    if ($null -eq $source) {
        Write-Host "[Role payload] Static visual not found for character $characterId ($type); skipping $($Row.name)."
        return $false
    }

    Copy-Item $source.FullName $Destination -Force
    return $true
}

try {
    & $SevenZip x $Archive "scripts/_assets/assets.swf" "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Could not extract assets.swf from Crystal Saga.rar"
    }

    $swf = Join-Path $tempPath "scripts\_assets\assets.swf"
    if (-not (Test-Path $swf)) {
        throw "Extracted assets.swf was not found: $swf"
    }

    $tool = Join-Path $root "tools\swf_ui_payload\swf_ui_payload.py"
    if (-not (Test-Path $tool)) {
        throw "SWF payload parser was not found: $tool"
    }

    $exitCode = Invoke-PythonScript -Arguments @($tool, $swf, "--output", $outputPath, "--exports", "symbol1998")
    if ($exitCode -ne 0) {
        throw "SWF payload parser failed for symbol1998"
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

    # Build a second manifest for first-frame static shapes/sprites/buttons that
    # are not already handled by the stable manual reconstruction. This recovers
    # genuine payload decorations and feature controls without duplicating the
    # known equipment/panel/progress/MainButton/attribute-control layer.
    $manifestTool = Join-Path $root "tools\swf_ui_payload\role_character_manifest.py"
    $manifest = Join-Path $outputDirectory "auto_manifest.tsv"
    if (Test-Path $manifestTool) {
        $exitCode = Invoke-PythonScript -Arguments @(
            $manifestTool,
            $swf,
            "--output", $manifest,
            "--exclude-ids", "1884", "276", "304", "263", "361", "89", "908", "1914", "1917"
        )
        if ($exitCode -eq 0 -and (Test-Path $manifest)) {
            Remove-Item $autoDirectory -Recurse -Force -ErrorAction SilentlyContinue
            New-Item -ItemType Directory -Force -Path $autoDirectory | Out-Null

            $rows = Import-Csv -Path $manifest -Delimiter "`t"
            $copied = 0
            foreach ($row in $rows) {
                $destination = Join-Path $autoDirectory ([string]$row.asset)
                if (Copy-AutoVisual -Row $row -Destination $destination) {
                    $copied++
                }
            }
            Write-Host "Role Character auto payload layer extracted: $copied / $($rows.Count) visuals."
        }
        else {
            Write-Host "[Role payload] Static visual manifest generation failed; continuing with the stable manual layer."
        }
    }
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}
