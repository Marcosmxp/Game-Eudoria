param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [string]$Output = "legacy_assets/runtime/ui/role_window/character",
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
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-role-character-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $outputPath, $tempPath | Out-Null

function Extract-Entry {
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

function Extract-Frames {
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

$buttonStates = [ordered]@{
    "up"      = "1_up.png"
    "over"    = "2_over.png"
    "down"    = "3_down.png"
    "hittest" = "4_hittest.png"
}

try {
    # PlayerFullInfoUIMC symbol1998 core display-list assets.
    Extract-Entry -Entry "shapes/1884.png" -Destination (Join-Path $outputPath "equipment_panel.png")

    # Some FFDec export folders include the AS3 linkage name after the character
    # id. Use the exact paths present in Crystal Saga.rar rather than assuming
    # every sprite folder is named only DefineSprite_<id>.
    Extract-Entry `
        -Entry "sprites/DefineSprite_276_playerUI.IconBarMC_playerUI.IconBarMC/1.png" `
        -Destination (Join-Path $outputPath "equipment_slot.png")
    Extract-Entry -Entry "sprites/DefineSprite_304/1.png" -Destination (Join-Path $outputPath "panel.png")
    Extract-Entry -Entry "sprites/DefineSprite_263/1.png" -Destination (Join-Path $outputPath "value_back.png")
    Extract-Entry `
        -Entry "sprites/DefineSprite_89_MainButton_MainButton/1.png" `
        -Destination (Join-Path $outputPath "main_button.png")

    # Both expBar and the secondary meter are symbol361 and are initialized at
    # frame 100 by the original PlayerFullInfo controller. Keep all 100 frames
    # now so later XP/progression binding does not require another extractor.
    Extract-Frames -CharacterId 361 -FrameCount 100 -Destination (Join-Path $outputPath "progress")

    $attributeButtons = [ordered]@{
        "attr_add" = 908
        "attr_remove" = 1914
        "attr_add_all" = 1917
    }
    foreach ($button in $attributeButtons.GetEnumerator()) {
        foreach ($state in $buttonStates.GetEnumerator()) {
            Extract-Entry `
                -Entry "buttons/DefineButton2_$($button.Value)/$($state.Value)" `
                -Destination (Join-Path $outputPath "$($button.Key)/$($state.Key).png")
        }
    }
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Role Character display-list assets extracted to $outputPath"
