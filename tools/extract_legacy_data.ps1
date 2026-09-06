param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [string]$Output = "legacy_assets/runtime/data",

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
$rawPath = Join-Path $outputPath "raw"
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-data-" + [guid]::NewGuid().ToString("N"))

New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
New-Item -ItemType Directory -Force -Path $rawPath | Out-Null
New-Item -ItemType Directory -Force -Path $tempPath | Out-Null

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
    return $source
}

function Clean-TsvText {
    param([AllowNull()][object]$Value)
    if ($null -eq $Value) {
        return ""
    }

    $text = [string]$Value
    $text = $text -replace "`t", " "
    $text = $text -replace "[`r`n]+", " "
    return $text.Trim()
}

try {
    $itlTemp = Extract-LegacyEntry -Entry "txt/itl.json" -Destination (Join-Path $rawPath "itl.json")
    $imbTemp = Extract-LegacyEntry -Entry "txt/imb.json" -Destination (Join-Path $rawPath "imb.json")
    $imwTemp = Extract-LegacyEntry -Entry "txt/imw.json" -Destination (Join-Path $rawPath "imw.json")

    # ResLoadModule.as proves these payloads are the original model sources:
    #   TaskManager.init(decodeJson(getInfContent("itl")))
    #   npc brief names come from imb
    #   WorldMapManager.init(JSON.decode(getInfContent("imw")))
    $tasks = Get-Content -Raw -Encoding UTF8 $itlTemp | ConvertFrom-Json
    $npcMaps = Get-Content -Raw -Encoding UTF8 $imbTemp | ConvertFrom-Json
    $worldMap = Get-Content -Raw -Encoding UTF8 $imwTemp | ConvertFrom-Json

    $mapNames = @{}
    foreach ($land in @($worldMap.lands)) {
        foreach ($region in @($land.regions)) {
            foreach ($map in @($region.maps)) {
                if ($null -ne $map.mapId) {
                    $mapNames[[string]$map.mapId] = [string]$map.mapName
                }
            }
        }
    }

    $npcNames = @{}
    foreach ($mapProperty in $npcMaps.PSObject.Properties) {
        $mapId = [string]$mapProperty.Name
        $mapValue = $mapProperty.Value
        if ($null -eq $mapValue -or $null -eq $mapValue.npcs) {
            continue
        }

        foreach ($npcProperty in $mapValue.npcs.PSObject.Properties) {
            $npcId = [string]$npcProperty.Name
            $npcValue = $npcProperty.Value
            if ($null -ne $npcValue -and $null -ne $npcValue.name) {
                $npcNames["$npcId@$mapId"] = [string]$npcValue.name
            }
        }
    }

    function Resolve-NpcDisplay {
        param([AllowNull()][object[]]$FullIds)

        if ($null -eq $FullIds) {
            return ""
        }

        $labels = New-Object System.Collections.Generic.List[string]
        foreach ($rawId in @($FullIds)) {
            if ($null -eq $rawId) {
                continue
            }

            $fullId = [string]$rawId
            $parts = $fullId.Split('@')
            $npcName = if ($npcNames.ContainsKey($fullId)) { $npcNames[$fullId] } else { $parts[0] }
            $mapName = ""
            if ($parts.Count -ge 2 -and $mapNames.ContainsKey($parts[1])) {
                $mapName = $mapNames[$parts[1]]
            }

            if ([string]::IsNullOrWhiteSpace($mapName)) {
                $labels.Add($npcName)
            }
            else {
                # Matches the original TaskTracerBoxUI convention: NPC-Map.
                $labels.Add("$npcName-$mapName")
            }
        }
        return ($labels -join "; ")
    }

    $builder = New-Object System.Text.StringBuilder
    [void]$builder.AppendLine("taskId`tkind`tname`tbrief`tdetail`treceiveAt`tfinishAt`treceiveRaw`tfinishRaw`tstarterTyria")

    $written = 0
    foreach ($task in @($tasks)) {
        $receiveRawList = @()
        if ($null -ne $task.receiveRule -and $null -ne $task.receiveRule.atAnyNpc) {
            $receiveRawList = @($task.receiveRule.atAnyNpc)
        }
        $finishRawList = @()
        if ($null -ne $task.finishRule -and $null -ne $task.finishRule.atAnyNpc) {
            $finishRawList = @($task.finishRule.atAnyNpc)
        }

        $starterTyria = $false
        if ([int]$task.kind -eq 1) {
            foreach ($npcId in $receiveRawList) {
                if ([string]$npcId -match '@a1$') {
                    $starterTyria = $true
                    break
                }
            }
        }

        $columns = @(
            (Clean-TsvText $task.taskId),
            (Clean-TsvText $task.kind),
            (Clean-TsvText $task.name),
            (Clean-TsvText $task.brief),
            (Clean-TsvText $task.detail),
            (Clean-TsvText (Resolve-NpcDisplay $receiveRawList)),
            (Clean-TsvText (Resolve-NpcDisplay $finishRawList)),
            (Clean-TsvText ($receiveRawList -join ";")),
            (Clean-TsvText ($finishRawList -join ";")),
            ($(if ($starterTyria) { "1" } else { "0" }))
        )
        [void]$builder.AppendLine(($columns -join "`t"))
        $written++
    }

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText((Join-Path $outputPath "tasks_ui.tsv"), $builder.ToString(), $utf8NoBom)

    Write-Host "Legacy task catalog normalized from txt/itl.json: $written tasks"
    Write-Host "Task UI data written to $(Join-Path $outputPath 'tasks_ui.tsv')"
    Write-Host "Raw itl/imb/imw payloads preserved under $rawPath"
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}
