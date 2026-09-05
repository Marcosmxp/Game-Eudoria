# Legacy Scanner

Indexes the local Crystal Saga resource dump without copying the multi-GB assets into Git.

## Run

Point the scanner at `resources_by_type\decoded` so the raw download cache is not indexed twice.

From the repository root on Windows:

```powershell
py tools/legacy_scanner/legacy_scanner.py `
  "C:\Users\Marcos\Downloads\CrystalSaga\resources_by_type\decoded" `
  --output generated/legacy
```

For a full SHA-256 pass (considerably slower on ~5.89 GB):

```powershell
py tools/legacy_scanner/legacy_scanner.py `
  "C:\Users\Marcos\Downloads\CrystalSaga\resources_by_type\decoded" `
  --output generated/legacy `
  --hash
```

Generated files:

- `legacy_catalog.json` - complete per-file catalogue
- `legacy_summary.json` - counts, sizes and map tile bounds
- `legacy_relationships.json` - inferred resource relationships
- `legacy_missing.json` - minimap/config relationship gaps

SWF scanning reads headers, stage dimensions, frame metadata, ExportAssets and SymbolClass tags for FWS/CWS files. ZWS (LZMA) resources are catalogued but deep parsing is deferred.
