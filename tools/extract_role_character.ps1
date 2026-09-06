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

function Write-FallbackReferenceBounds {
    param([Parameter(Mandatory = $true)][string]$ReferencePath)

    # PlayerFullInfoUIMC symbol1998 is centered on pointChildUI. These local
    # bounds are recovered from the stable display-list reconstruction already
    # verified in the payload: the equipment panel starts at x=-211.75 and the
    # top controls start around y=-230. Only left/top are required by the native
    # renderer; right/bottom are derived from the actual FFDec raster size.
    $left = -211.75
    $top = -230.0
    $width = 425
    $height = 555

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
        Write-Host "[Role Character] Could not read reference PNG dimensions; using payload fallback dimensions."
    }

    $right = $left + $width
    $bottom = $top + $height
    $boundsPath = Join-Path $outputPath "reference_bounds.tsv"
    @(
        "left`ttop`tright`tbottom"
        "$left`t$top`t$right`t$bottom"
    ) | Set-Content -Path $boundsPath -Encoding UTF8

    Write-Host "Role Character fallback reference bounds written to $boundsPath"
}

$buttonStates = [ordered]@{
    "up"      = "1_up.png"
    "over"    = "2_over.png"
    "down"    = "3_down.png"
    "hittest" = "4_hittest.png"
}

$referencePath = Join-Path $outputPath "reference.png"

try {
    # Character uses the complete PlayerFullInfoUIMC first-frame raster as the
    # visual baseline. Resolve the FFDec linkage-suffixed folder from the real
    # archive rather than assuming a fixed folder name.
    $referenceEntry = Resolve-ArchiveEntry -Pattern '^sprites/DefineSprite_1998(?:_[^/]*)?/1\.png$'
    if (-not $referenceEntry) {
        throw "PlayerFullInfoUIMC symbol1998 reference raster was not found in Crystal Saga.rar"
    }
    Extract-Entry -Entry $referenceEntry -Destination $referencePath

    # Always create usable bounds before optional SWF diagnostics run. This keeps
    # OneClick functional even if the lightweight Python parser encounters an
    # unsupported PlaceObject feature in this unusually complex symbol.
    Write-FallbackReferenceBounds -ReferencePath $referencePath

    # Keep individual assets as diagnostics/future interactive decomposition.
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

# The SWF dump is useful for diagnostics and can replace the fallback bounds
# with more exact values, but it must never stop the game build. The runtime has
# already received reference.png + reference_bounds.tsv above.
$dumpTool = Join-Path $root "tools\dump_role_character_payload.ps1"
if (Test-Path $dumpTool) {
    try {
        powershell.exe -NoProfile -ExecutionPolicy Bypass -File $dumpTool -Archive $Archive -SevenZip $SevenZip
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[Role Character] Exact SWF payload dump failed; using the extracted reference raster and fallback bounds."
        }
    }
    catch {
        Write-Host "[Role Character] Exact SWF payload dump skipped: $($_.Exception.Message)"
    }
}
