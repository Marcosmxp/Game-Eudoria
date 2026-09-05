param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [string]$Output = "legacy_assets/reference/ui",

    [string]$SevenZip = "7z"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $Archive)) {
    throw "Archive not found: $Archive"
}

if (-not (Get-Command $SevenZip -ErrorAction SilentlyContinue)) {
    throw "7-Zip was not found. Install 7-Zip or pass -SevenZip with the full path to 7z.exe."
}

$root = (Resolve-Path ".").Path
$outputPath = Join-Path $root $Output
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-ui-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
New-Item -ItemType Directory -Force -Path $tempPath | Out-Null

$entries = @{
    "scripts/_assets/assets.swf" = "assets.swf"
    "sprites/DefineSprite_3550_playerUI.PlayerInfoUIMC_playerUI.PlayerInfoUIMC/1.png" = "player_info.reference.png"
    "sprites/DefineSprite_4343_playerUI.GameInfoUIMC_playerUI.GameInfoUIMC/1.png" = "game_info.reference.png"
    "sprites/DefineSprite_4131_playerUI.ControlBarUIMC_playerUI.ControlBarUIMC/1.png" = "control_bar.reference.png"
    "sprites/DefineSprite_1825_playerUI.SmallMapUIMC_playerUI.SmallMapUIMC/1.png" = "small_map.reference.png"
    "sprites/DefineSprite_4135_playerUI.TaskTracerUIMC_playerUI.TaskTracerUIMC/1.png" = "task_tracer.reference.png"
}

try {
    foreach ($entry in $entries.GetEnumerator()) {
        & $SevenZip x $Archive $entry.Key "-o$tempPath" -y | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "7-Zip failed while extracting $($entry.Key)"
        }

        $source = Join-Path $tempPath ($entry.Key -replace '/', [IO.Path]::DirectorySeparatorChar)
        if (-not (Test-Path $source)) {
            throw "Expected extracted file was not found: $source"
        }
        Copy-Item $source (Join-Path $outputPath $entry.Value) -Force
    }
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Legacy HUD reference files extracted to $outputPath"
Write-Host "These PNGs are reconstruction references; runtime UI will be rebuilt from the SWF display list."
