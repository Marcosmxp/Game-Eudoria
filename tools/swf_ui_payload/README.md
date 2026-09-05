# SWF UI Payload Extractor

Reads the recovered `assets.swf` directly and exports the first-frame Flash display list used by Eudoria's legacy UI reconstruction.

It does not run Flash and has no third-party Python dependencies.

## Confirmed main HUD exports

- `symbol3550` - `playerUI.PlayerInfoUIMC`
- `symbol4343` - `playerUI.GameInfoUIMC` (chat/message HUD)
- `symbol4131` - `playerUI.ControlBarUIMC`
- `symbol1825` - `playerUI.SmallMapUIMC`
- `symbol4135` - `playerUI.TaskTracerUIMC`

## Run

```powershell
py tools/swf_ui_payload/swf_ui_payload.py `
  "C:\path\to\assets.swf" `
  --output generated/ui/hud.legacy.json
```

The generated payload preserves child depth, Character ID, instance name, scale, skew/rotation matrix and X/Y translation. Flash translations are converted from twips to pixels.
