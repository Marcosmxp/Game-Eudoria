param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [string]$Output = "legacy_assets/reference/ui",

    [string]$RuntimeOutput = "legacy_assets/runtime/ui/control_bar",

    [string]$PlayerInfoOutput = "legacy_assets/runtime/ui/player_info",

    [string]$GameInfoOutput = "legacy_assets/runtime/ui/game_info",

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
$runtimeOutputPath = Join-Path $root $RuntimeOutput
$playerInfoOutputPath = Join-Path $root $PlayerInfoOutput
$gameInfoOutputPath = Join-Path $root $GameInfoOutput
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-ui-" + [guid]::NewGuid().ToString("N"))

New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
New-Item -ItemType Directory -Force -Path $runtimeOutputPath | Out-Null
New-Item -ItemType Directory -Force -Path $playerInfoOutputPath | Out-Null
New-Item -ItemType Directory -Force -Path $gameInfoOutputPath | Out-Null
New-Item -ItemType Directory -Force -Path $tempPath | Out-Null

function Extract-LegacyEntry {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Entry,

        [Parameter(Mandatory = $true)]
        [string]$Destination
    )

    & $SevenZip x $Archive $Entry "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip failed while extracting $Entry"
    }

    $source = Join-Path $tempPath ($Entry -replace '/', [IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path $source)) {
        throw "Expected extracted file was not found: $source"
    }

    $destinationDirectory = Split-Path $Destination -Parent
    if ($destinationDirectory) {
        New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
    }

    Copy-Item $source $Destination -Force
}

function Extract-PlayerBarFrames {
    param(
        [Parameter(Mandatory = $true)]
        [int]$CharacterId,

        [Parameter(Mandatory = $true)]
        [string]$Destination
    )

    $archiveDirectory = "sprites/DefineSprite_$CharacterId"
    & $SevenZip x $Archive "$archiveDirectory/*" "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip failed while extracting $archiveDirectory"
    }

    $sourceDirectory = Join-Path $tempPath ($archiveDirectory -replace '/', [IO.Path]::DirectorySeparatorChar)
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null

    for ($frame = 1; $frame -le 100; $frame++) {
        $source = Join-Path $sourceDirectory "$frame.png"
        if (-not (Test-Path $source)) {
            throw "Missing PlayerInfo frame: $source"
        }
        Copy-Item $source (Join-Path $Destination "$frame.png") -Force
    }
}

$references = @{
    "scripts/_assets/assets.swf" = "assets.swf"
    "sprites/DefineSprite_3550_playerUI.PlayerInfoUIMC_playerUI.PlayerInfoUIMC/1.png" = "player_info.reference.png"
    "sprites/DefineSprite_4343_playerUI.GameInfoUIMC_playerUI.GameInfoUIMC/1.png" = "game_info.reference.png"
    "sprites/DefineSprite_4131_playerUI.ControlBarUIMC_playerUI.ControlBarUIMC/1.png" = "control_bar.reference.png"
    "sprites/DefineSprite_1825_playerUI.SmallMapUIMC_playerUI.SmallMapUIMC/1.png" = "small_map.reference.png"
    "sprites/DefineSprite_4135_playerUI.TaskTracerUIMC_playerUI.TaskTracerUIMC/1.png" = "task_tracer.reference.png"
    "sprites/DefineSprite_5665_playerUI.TaskTracerBoxUIMC_playerUI.TaskTracerBoxUIMC/1.png" = "task_tracer_box.reference.png"
}

$controlBarButtons = [ordered]@{
    "cmdBag"      = 3997
    "cmdQuest"    = 4001
    "cmdFamily"   = 4005
    "cmdPet"      = 4006
    "cmdRole"     = 4010
    "cmdSkill"    = 4014
    "cmdFriend"   = 4018
    "cmdTeam"     = 4022
    "cmdRide"     = 4024
    "cmdBot"      = 4028
    "cmdBlessGod" = 4032
    "cmdSys"      = 4036
    "cmdWing"     = 4038
    "up"          = 1821
    "down"        = 1818
}

$buttonStates = [ordered]@{
    "up"      = "1_up.png"
    "over"    = "2_over.png"
    "down"    = "3_down.png"
    "hittest" = "4_hittest.png"
}

$gameInfoButtons = [ordered]@{
    "content_toggle" = 4327
    "enter"          = 4336
    "face"           = 4340
}

try {
    foreach ($entry in $references.GetEnumerator()) {
        Extract-LegacyEntry `
            -Entry $entry.Key `
            -Destination (Join-Path $outputPath $entry.Value)
    }

    # ControlBar runtime is assembled from the actual symbol4131 display list.
    # reference/control_bar.reference.png remains F2-only and is not used by ControlBar.cpp.
    Extract-LegacyEntry `
        -Entry "shapes/3993.png" `
        -Destination (Join-Path $runtimeOutputPath "base.png")

    Extract-LegacyEntry `
        -Entry "sprites/DefineSprite_4130/1.png" `
        -Destination (Join-Path $runtimeOutputPath "total_icon.png")

    Extract-LegacyEntry `
        -Entry "sprites/DefineSprite_257/1.png" `
        -Destination (Join-Path $runtimeOutputPath "sound/on.png")

    Extract-LegacyEntry `
        -Entry "sprites/DefineSprite_257/2.png" `
        -Destination (Join-Path $runtimeOutputPath "sound/off.png")

    foreach ($button in $controlBarButtons.GetEnumerator()) {
        foreach ($state in $buttonStates.GetEnumerator()) {
            $entry = "buttons/DefineButton2_$($button.Value)/$($state.Value)"
            $destination = Join-Path $runtimeOutputPath "$($button.Key)/$($state.Key).png"
            Extract-LegacyEntry -Entry $entry -Destination $destination
        }
    }

    Extract-LegacyEntry `
        -Entry "sprites/DefineSprite_4318_somcUI_fla.ScopeButton_423_somcUI_fla.ScopeButton_423/1.png" `
        -Destination (Join-Path $gameInfoOutputPath "scope_button/normal.png")

    Extract-LegacyEntry `
        -Entry "sprites/DefineSprite_4318_somcUI_fla.ScopeButton_423_somcUI_fla.ScopeButton_423/2.png" `
        -Destination (Join-Path $gameInfoOutputPath "scope_button/active.png")

    foreach ($button in $gameInfoButtons.GetEnumerator()) {
        foreach ($state in $buttonStates.GetEnumerator()) {
            $entry = "buttons/DefineButton2_$($button.Value)/$($state.Value)"
            $destination = Join-Path $gameInfoOutputPath "$($button.Key)/$($state.Key).png"
            Extract-LegacyEntry -Entry $entry -Destination $destination
        }
    }

    Extract-PlayerBarFrames `
        -CharacterId 674 `
        -Destination (Join-Path $playerInfoOutputPath "hp")

    Extract-PlayerBarFrames `
        -CharacterId 269 `
        -Destination (Join-Path $playerInfoOutputPath "mp")
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Legacy HUD references extracted to $outputPath"
Write-Host "ControlBar display-list assets extracted to $runtimeOutputPath"
Write-Host "PlayerInfo HP/MP frames extracted to $playerInfoOutputPath"
Write-Host "GameInfo runtime assets extracted to $gameInfoOutputPath"
