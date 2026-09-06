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
    Write-Host "[Role payload] Python not found; exact symbol1998 diagnostic dump skipped."
    exit 0
}

$root = (Resolve-Path ".").Path
$outputPath = Join-Path $root $Output
$outputDirectory = Split-Path $outputPath -Parent
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-role-payload-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $outputDirectory, $tempPath | Out-Null

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

    if ($python -eq "py") {
        & $python -3 $tool $swf --output $outputPath --exports symbol1998
    }
    else {
        & $python $tool $swf --output $outputPath --exports symbol1998
    }
    if ($LASTEXITCODE -ne 0) {
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
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}
