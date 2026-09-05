param(
    [Parameter(Mandatory = $true)]
    [string]$Archive,

    [Parameter(Mandatory = $true)]
    [string]$MapId,

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

$normalized = $MapId.Trim()
if (-not $normalized.StartsWith("p")) {
    $normalized = "p$normalized"
}
if (-not $normalized.EndsWith(".jpg")) {
    $normalized = "$normalized.jpg"
}

$root = (Resolve-Path ".").Path
$outputPath = Join-Path $root $Output
$tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("eudoria-minimap-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $tempPath | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $outputPath -Parent) | Out-Null

try {
    & $SevenZip x $Archive $normalized "-o$tempPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip failed while extracting $normalized"
    }

    $source = Get-ChildItem -Path $tempPath -Recurse -File |
        Where-Object { $_.Name -eq $normalized } |
        Select-Object -First 1

    if (-not $source) {
        throw "Minimap not found in archive: $normalized"
    }

    Copy-Item $source.FullName $outputPath -Force
}
finally {
    Remove-Item $tempPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Minimap $normalized extracted to $outputPath"
Write-Host "img.rar supplies minimap/reference images only. Playable world maps remain in the local multi-GB asset set."
