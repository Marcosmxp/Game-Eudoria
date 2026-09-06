param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [string]$Output = "legacy_assets/reference/ui",
    [string]$RuntimeOutput = "legacy_assets/runtime/ui/control_bar",
    [string]$PlayerInfoOutput = "legacy_assets/runtime/ui/player_info",
    [string]$GameInfoOutput = "legacy_assets/runtime/ui/game_info",
    [string]$SmallMapOutput = "legacy_assets/runtime/ui/small_map",
    [string]$TaskTracerOutput = "legacy_assets/runtime/ui/task_tracer",
    [string]$RoleWindowOutput = "legacy_assets/runtime/ui/role_window",
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
$smallMapOutputPath = Join-Path $root $SmallMapOutput
$taskTracerOutputPath = Join-Path $root $TaskTracerOutput
$roleWindowOutputPath = Join-Path $root $RoleWindowOutput
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-ui-" + [guid]::NewGuid().ToString("N"))

@($outputPath, $runtimeOutputPath, $playerInfoOutputPath, $gameInfoOutputPath, $smallMapOutputPath, $taskTracerOutputPath, $roleWindowOutputPath, $tempPath) |
    ForEach-Object { New-Item -ItemType Directory -Force -Path $_ | Out-Null }

function Extract-LegacyEntry {
    param(
        [Parameter(Mandatory = $true)][string]$Entry,
        [Parameter(Mandatory = $true)][string]$Destination
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

function Extract-NumberedSpriteFrames {
    param(
        [Parameter(Mandatory = $true)][int]$CharacterId,
        [Parameter(Mandatory = $true)][int]$FrameCount,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $archiveDirectory = "sprites/DefineSprite_$CharacterId"
    & $SevenZip x $Archive "$archiveDirectory/*" "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip failed while extracting $archiveDirectory"
    }

    $sourceDirectory = Join-Path $tempPath ($archiveDirectory -replace '/', [IO.Path]::DirectorySeparatorChar)
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null

    for ($frame = 1; $frame -le $FrameCount; $frame++) {
        $source = Join-Path $sourceDirectory "$frame.png"
        if (-not (Test-Path $source)) {
            throw "Missing frame $frame for character $CharacterId"
        }
        Copy-Item $source (Join-Path $Destination "$frame.png") -Force
    }
}

function Flip-ImageHorizontal {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    Add-Type -AssemblyName System.Drawing
    $destinationDirectory = Split-Path $Destination -Parent
    if ($destinationDirectory) {
        New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
    }

    $bitmap = New-Object System.Drawing.Bitmap($Source)
    try {
        $bitmap.RotateFlip([System.Drawing.RotateFlipType]::RotateNoneFlipX)
        $bitmap.Save($Destination, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $bitmap.Dispose()
    }
}

$buttonStates = [ordered]@{
    "up"      = "1_up.png"
    "over"    = "2_over.png"
    "down"    = "3_down.png"
    "hittest" = "4_hittest.png"
}

$references = @{
    "scripts/_assets/assets.swf" = "assets.swf"
    "sprites/DefineSprite_3550_playerUI.PlayerInfoUIMC_playerUI.PlayerInfoUIMC/1.png" = "player_info.reference.png"
    "sprites/DefineSprite_4343_playerUI.GameInfoUIMC_playerUI.GameInfoUIMC/1.png" = "game_info.reference.png"
    "sprites/DefineSprite_4131_playerUI.ControlBarUIMC_playerUI.ControlBarUIMC/1.png" = "control_bar.reference.png"
    "sprites/DefineSprite_1825_playerUI.SmallMapUIMC_playerUI.SmallMapUIMC/1.png" = "small_map.reference.png"
    "sprites/DefineSprite_4135_playerUI.TaskTracerUIMC_playerUI.TaskTracerUIMC/1.png" = "task_tracer.reference.png"
    "sprites/DefineSprite_5665_playerUI.TaskTracerBoxUIMC_playerUI.TaskTracerBoxUIMC/1.png" = "task_tracer_box.reference.png"
    "sprites/DefineSprite_4930_playerUI.PlayerBoxUIMC_playerUI.PlayerBoxUIMC/1.png" = "role_window.reference.png"
    "sprites/DefineSprite_1998_playerUI.PlayerFullInfoUIMC_playerUI.PlayerFullInfoUIMC/1.png" = "role_character.reference.png"
}

$controlBarButtons = [ordered]@{
    "cmdBag" = 3997; "cmdQuest" = 4001; "cmdFamily" = 4005; "cmdPet" = 4006
    "cmdRole" = 4010; "cmdSkill" = 4014; "cmdFriend" = 4018; "cmdTeam" = 4022
    "cmdRide" = 4024; "cmdBot" = 4028; "cmdBlessGod" = 4032; "cmdSys" = 4036
    "cmdWing" = 4038; "up" = 1821; "down" = 1818
}

$smallMapButtons = [ordered]@{
    "zoom_out" = 1638; "zoom_in" = 1642; "map" = 1651; "world_map" = 1660
    "shop" = 1670; "days_prompt" = 1673; "ranking" = 1676; "day_bonus" = 1679
    "drg_lottery" = 1691; "result" = 1701; "collapse" = 1821; "expand" = 1818
}

$gameInfoButtons = [ordered]@{
    "content_toggle" = 4327
    "enter" = 4336
    "face" = 4340
}

$playerInfoButtons = [ordered]@{
    "fate_skill" = 3529
    "pet_action" = 3532
}

try {
    foreach ($entry in $references.GetEnumerator()) {
        Extract-LegacyEntry -Entry $entry.Key -Destination (Join-Path $outputPath $entry.Value)
    }

    # ControlBar: symbol4131 display-list assets.
    Extract-LegacyEntry -Entry "shapes/3993.png" -Destination (Join-Path $runtimeOutputPath "base.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_4130/1.png" -Destination (Join-Path $runtimeOutputPath "total_icon.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_257/1.png" -Destination (Join-Path $runtimeOutputPath "sound/on.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_257/2.png" -Destination (Join-Path $runtimeOutputPath "sound/off.png")
    foreach ($button in $controlBarButtons.GetEnumerator()) {
        foreach ($state in $buttonStates.GetEnumerator()) {
            Extract-LegacyEntry `
                -Entry "buttons/DefineButton2_$($button.Value)/$($state.Value)" `
                -Destination (Join-Path $runtimeOutputPath "$($button.Key)/$($state.Key).png")
        }
    }

    # SmallMap: symbol1825 display-list assets. totalIcon remains independent.
    Extract-LegacyEntry -Entry "shapes/1632.png" -Destination (Join-Path $smallMapOutputPath "base.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_1634/1.png" -Destination (Join-Path $smallMapOutputPath "player_center.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_1647/1.png" -Destination (Join-Path $smallMapOutputPath "online_bonus.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_1656/1.png" -Destination (Join-Path $smallMapOutputPath "remote_display.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_1683/1.png" -Destination (Join-Path $smallMapOutputPath "skill_effect.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_1694/1.png" -Destination (Join-Path $smallMapOutputPath "misc_1694.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_1815/1.png" -Destination (Join-Path $smallMapOutputPath "total_icon.png")
    foreach ($button in $smallMapButtons.GetEnumerator()) {
        foreach ($state in $buttonStates.GetEnumerator()) {
            Extract-LegacyEntry `
                -Entry "buttons/DefineButton2_$($button.Value)/$($state.Value)" `
                -Destination (Join-Path $smallMapOutputPath "$($button.Key)/$($state.Key).png")
        }
    }

    # TaskTracer: rebuild symbol4135 and symbol5665 from their real children.
    # The old task_tracer.reference.png stays F2-only. Using it directly caused
    # the Flash component placeholder scrollbar to appear as a large white bar.
    Extract-LegacyEntry -Entry "shapes/303.png" -Destination (Join-Path $taskTracerOutputPath "title_box.png")
    Extract-LegacyEntry -Entry "shapes/432.png" -Destination (Join-Path $taskTracerOutputPath "tab/normal.png")
    Extract-LegacyEntry -Entry "shapes/435.png" -Destination (Join-Path $taskTracerOutputPath "tab/active.png")
    Extract-LegacyEntry -Entry "shapes/5661.png" -Destination (Join-Path $taskTracerOutputPath "task_separator.png")

    # Native UIScrollBar skins used by fl.controls.ScrollBar.defaultStyles.
    Extract-LegacyEntry -Entry "shapes/191.png" -Destination (Join-Path $taskTracerOutputPath "scroll/track.png")
    Extract-LegacyEntry -Entry "shapes/194.png" -Destination (Join-Path $taskTracerOutputPath "scroll/up.png")
    Extract-LegacyEntry -Entry "shapes/196.png" -Destination (Join-Path $taskTracerOutputPath "scroll/down.png")
    Extract-LegacyEntry -Entry "shapes/198.png" -Destination (Join-Path $taskTracerOutputPath "scroll/thumb.png")
    Extract-LegacyEntry -Entry "shapes/208.png" -Destination (Join-Path $taskTracerOutputPath "scroll/thumb_icon.png")

    # Role / Character container: PlayerBoxUIMC symbol4930. This is rebuilt
    # from the actual first-frame display list instead of the FFDec composite.
    Extract-LegacyEntry -Entry "shapes/153.png" -Destination (Join-Path $roleWindowOutputPath "background.png")
    Extract-LegacyEntry -Entry "shapes/186.png" -Destination (Join-Path $roleWindowOutputPath "title_box.png")
    Extract-LegacyEntry -Entry "shapes/432.png" -Destination (Join-Path $roleWindowOutputPath "tab/normal.png")
    Extract-LegacyEntry -Entry "shapes/435.png" -Destination (Join-Path $roleWindowOutputPath "tab/active.png")
    foreach ($state in $buttonStates.GetEnumerator()) {
        Extract-LegacyEntry `
            -Entry "buttons/DefineButton2_172/$($state.Value)" `
            -Destination (Join-Path $roleWindowOutputPath "close/$($state.Key).png")
    }

    # GameInfo / chat assets.
    Extract-LegacyEntry `
        -Entry "sprites/DefineSprite_4318_somcUI_fla.ScopeButton_423_somcUI_fla.ScopeButton_423/1.png" `
        -Destination (Join-Path $gameInfoOutputPath "scope_button/normal.png")
    Extract-LegacyEntry `
        -Entry "sprites/DefineSprite_4318_somcUI_fla.ScopeButton_423_somcUI_fla.ScopeButton_423/2.png" `
        -Destination (Join-Path $gameInfoOutputPath "scope_button/active.png")
    foreach ($button in $gameInfoButtons.GetEnumerator()) {
        foreach ($state in $buttonStates.GetEnumerator()) {
            Extract-LegacyEntry `
                -Entry "buttons/DefineButton2_$($button.Value)/$($state.Value)" `
                -Destination (Join-Path $gameInfoOutputPath "$($button.Key)/$($state.Key).png")
        }
    }

    # PlayerInfo: reconstruct symbol3550 from the real display list instead of
    # player_info.reference.png. The reference stays F2-only.
    Extract-NumberedSpriteFrames -CharacterId 3505 -FrameCount 2 -Destination (Join-Path $playerInfoOutputPath "background")
    Extract-LegacyEntry -Entry "shapes/3507.png" -Destination (Join-Path $playerInfoOutputPath "pet_frame.png")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_3509/1.png" -Destination (Join-Path $playerInfoOutputPath "resource_back.png")
    Flip-ImageHorizontal `
        -Source (Join-Path $playerInfoOutputPath "resource_back.png") `
        -Destination (Join-Path $playerInfoOutputPath "resource_back_flip.png")
    Extract-LegacyEntry -Entry "shapes/3511.png" -Destination (Join-Path $playerInfoOutputPath "divider.png")

    Extract-NumberedSpriteFrames -CharacterId 3514 -FrameCount 100 -Destination (Join-Path $playerInfoOutputPath "reserve_hp")
    Extract-NumberedSpriteFrames -CharacterId 3517 -FrameCount 100 -Destination (Join-Path $playerInfoOutputPath "reserve_mp")
    Extract-NumberedSpriteFrames -CharacterId 3519 -FrameCount 100 -Destination (Join-Path $playerInfoOutputPath "reserve_mask")
    New-Item -ItemType Directory -Force -Path (Join-Path $playerInfoOutputPath "reserve_mask_flip") | Out-Null
    for ($frame = 1; $frame -le 100; $frame++) {
        Flip-ImageHorizontal `
            -Source (Join-Path $playerInfoOutputPath "reserve_mask/$frame.png") `
            -Destination (Join-Path $playerInfoOutputPath "reserve_mask_flip/$frame.png")
    }

    Extract-NumberedSpriteFrames -CharacterId 674 -FrameCount 100 -Destination (Join-Path $playerInfoOutputPath "hp")
    Extract-NumberedSpriteFrames -CharacterId 269 -FrameCount 100 -Destination (Join-Path $playerInfoOutputPath "mp")

    foreach ($button in $playerInfoButtons.GetEnumerator()) {
        foreach ($state in $buttonStates.GetEnumerator()) {
            Extract-LegacyEntry `
                -Entry "buttons/DefineButton2_$($button.Value)/$($state.Value)" `
                -Destination (Join-Path $playerInfoOutputPath "$($button.Key)/$($state.Key).png")
        }
    }

    Extract-NumberedSpriteFrames -CharacterId 3541 -FrameCount 3 -Destination (Join-Path $playerInfoOutputPath "fps")
    Extract-NumberedSpriteFrames -CharacterId 3546 -FrameCount 3 -Destination (Join-Path $playerInfoOutputPath "ping")
    Extract-LegacyEntry -Entry "sprites/DefineSprite_3549/1.png" -Destination (Join-Path $playerInfoOutputPath "team_leader.png")
    Extract-LegacyEntry `
        -Entry "sprites/DefineSprite_5882_img.HeadIcon_000_img.HeadIcon_000/1.png" `
        -Destination (Join-Path $playerInfoOutputPath "head/default.png")
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Legacy HUD references extracted to $outputPath"
Write-Host "ControlBar display-list assets extracted to $runtimeOutputPath"
Write-Host "SmallMap display-list assets extracted to $smallMapOutputPath"
Write-Host "TaskTracer display-list assets extracted to $taskTracerOutputPath"
Write-Host "Role window display-list assets extracted to $roleWindowOutputPath"
Write-Host "PlayerInfo display-list assets extracted to $playerInfoOutputPath"
Write-Host "GameInfo runtime assets extracted to $gameInfoOutputPath"
