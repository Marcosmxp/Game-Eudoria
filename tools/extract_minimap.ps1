param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [string]$MapId = "",

    [string]$Output = "legacy_assets/runtime/minimap/current.jpg",

    [string]$SevenZip = "7z"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $Archive)) {
    throw "Archive not found: $Archive"
}

if (-not (Get-Command $SevenZip -ErrorAction SilentlyContinue)) {
    throw "7-Zip was not found. Install 7-Zip or pass -SevenZip with the full path to 7z.exe."
}

$requestedName = $null
if ($MapId.Trim()) {
    $requestedName = $MapId.Trim()
    if (-not $requestedName.StartsWith("p")) {
        $requestedName = "p$requestedName"
    }
    if (-not $requestedName.EndsWith(".jpg")) {
        $requestedName = "$requestedName.jpg"
    }
}

# Enumerate the archive first. img.rar stores minimaps under paths such as
# img/pma1.jpg rather than necessarily at the archive root, so extraction by
# bare file name is not reliable.
$entries = @(
    & $SevenZip l -slt $Archive |
        Where-Object { $_ -like "Path = *" } |
        ForEach-Object { $_.Substring(7).Trim() } |
        Where-Object {
            $leaf = [IO.Path]::GetFileName($_)
            $leaf -match '^p.+\.jpg$'
        }
)

if (-not $entries -or $entries.Count -eq 0) {
    throw "No minimap images matching p*.jpg were found in img.rar"
}

$selectedEntry = $null
if ($requestedName) {
    $selectedEntry = $entries |
        Where-Object { [IO.Path]::GetFileName($_) -ieq $requestedName } |
        Select-Object -First 1

    if (-not $selectedEntry) {
        Write-Warning "Requested minimap $requestedName was not found. Selecting the first available minimap for UI preview instead."
    }
}

if (-not $selectedEntry) {
    $selectedEntry = $entries | Sort-Object | Select-Object -First 1
}

$selectedName = [IO.Path]::GetFileName($selectedEntry)
$root = (Resolve-Path ".").Path
$outputPath = Join-Path $root $Output
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-minimap-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $tempPath | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $outputPath -Parent) | Out-Null

try {
    & $SevenZip x $Archive $selectedEntry "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip failed while extracting $selectedEntry"
    }

    $source = Get-ChildItem -Path $tempPath -Recurse -File |
        Where-Object { $_.Name -ieq $selectedName } |
        Select-Object -First 1

    if (-not $source) {
        throw "Expected minimap was not found after extraction: $selectedEntry"
    }

    Copy-Item $source.FullName $outputPath -Force
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Minimap $selectedEntry extracted to $outputPath"
Write-Host "This is a UI preview minimap from img.rar. Playable world maps remain in the local multi-GB asset set."
