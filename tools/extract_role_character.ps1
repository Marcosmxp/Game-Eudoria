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

function Resolve-ArchiveEntry {
    param([Parameter(Mandatory = $true)][string]$Pattern)

    $listing = & $SevenZip l -slt $Archive
    if ($LASTEXITCODE -ne 0) {
        return $null
    }
    foreach ($line in $listing) {
        if ($line -like "Path = *") {
            $entry = $line.Substring(7).Replace('\', '/')
            if ($entry -match $Pattern) {
                return $entry
            }
        }
    }
    return $null
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

function Write-CalibratedReferenceBounds {
    param([Parameter(Mandatory = $true)][string]$ReferencePath)

    # FFDec exports symbol1998 on a canvas with transparent left/top padding.
    # The visible equipment panel (shape1884) is placed by the SWF at:
    #   shape bounds  = -186.75..50.75, -164.75..74.75
    #   placement     = -25, 11
    # so its global top-left is (-211.75, -153.75).
    # In the original symbol1998 raster the same panel starts at pixel (121,77).
    # Therefore the full FFDec canvas origin is exactly:
    #   X = -211.75 - 121 = -332.75
    #   Y = -153.75 -  77 = -230.75
    $left = -332.75
    $top = -230.75
    $width = 563
    $height = 723

    try {
        Add-Type -AssemblyName System.Drawing -ErrorAction Stop
        $image = [System.Drawing.Image]::FromFile($ReferencePath)
        try {
            $width = $image.Width
            $height = $image.Height
        }
        finally {
            $image.Dispose()
        }
    }
    catch {
        Write-Host "[Role Character] Could not read reference PNG dimensions; using the known symbol1998 canvas size."
    }

    $right = $left + $width
    $bottom = $top + $height
    $boundsPath = Join-Path $outputPath "reference_bounds.tsv"
    @(
        "left`ttop`tright`tbottom"
        "$left`t$top`t$right`t$bottom"
    ) | Set-Content -Path $boundsPath -Encoding UTF8

    Write-Host "Role Character calibrated reference bounds written to $boundsPath"
}

$buttonStates = [ordered]@{
    "up"      = "1_up.png"
    "over"    = "2_over.png"
    "down"    = "3_down.png"
    "hittest" = "4_hittest.png"
}

$referencePath = Join-Path $outputPath "reference.png"

try {
    $referenceEntry = Resolve-ArchiveEntry -Pattern '^sprites/DefineSprite_1998(?:_[^/]*)?/1\.png$'
    if (-not $referenceEntry) {
        throw "PlayerFullInfoUIMC symbol1998 reference raster was not found in Crystal Saga.rar"
    }
    Extract-Entry -Entry $referenceEntry -Destination $referencePath
    Write-CalibratedReferenceBounds -ReferencePath $referencePath

    # Keep these assets available for the next pass where interactive states are
    # layered on top of the visually locked symbol1998 base. They are diagnostics
    # only in the current fidelity pass and are not used to reconstruct the base.
    Extract-Entry -Entry "shapes/1884.png" -Destination (Join-Path $outputPath "equipment_panel.png")
    Extract-Entry `
        -Entry "sprites/DefineSprite_276_playerUI.IconBarMC_playerUI.IconBarMC/1.png" `
        -Destination (Join-Path $outputPath "equipment_slot.png")
    Extract-Entry -Entry "sprites/DefineSprite_304/1.png" -Destination (Join-Path $outputPath "panel.png")
    Extract-Entry -Entry "sprites/DefineSprite_263/1.png" -Destination (Join-Path $outputPath "value_back.png")
    Extract-Entry `
        -Entry "sprites/DefineSprite_89_MainButton_MainButton/1.png" `
        -Destination (Join-Path $outputPath "main_button.png")

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

Write-Host "Role Character payload reference and diagnostic assets extracted to $outputPath"

# Diagnostics may still generate visual/text manifests for later interactive
# reconstruction, but they are not allowed to redefine the FFDec canvas origin
# used by the live UI.
$dumpTool = Join-Path $root "tools\dump_role_character_payload.ps1"
if (Test-Path $dumpTool) {
    try {
        powershell.exe -NoProfile -ExecutionPolicy Bypass -File $dumpTool -Archive $Archive -SevenZip $SevenZip
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[Role Character] Exact SWF diagnostic dump failed; continuing with the original symbol1998 raster."
        }
    }
    catch {
        Write-Host "[Role Character] Exact SWF diagnostic dump skipped: $($_.Exception.Message)"
    }
}

# dump_role_character_payload.ps1 can write reference_bounds.tsv as a recursive
# symbol bound. That value is useful diagnostically but is NOT the FFDec canvas
# origin. Rewrite the calibrated raster coordinates last so runtime placement is
# deterministic on every OneClick run.
Write-CalibratedReferenceBounds -ReferencePath $referencePath
