# Legacy HUD reconstruction

This document records facts recovered from the supplied Crystal Saga SWF payload. They are reconstruction inputs, not estimates from screenshots.

## Main HUD roots

| Eudoria component | Legacy class | Export | Root position on 1200x640 stage |
|---|---|---:|---:|
| Player status | `playerUI.PlayerInfoUIMC` | `symbol3550` | `(0, 0)` |
| Chat/message HUD | `playerUI.GameInfoUIMC` | `symbol4343` | `(0, 570)` |
| Bottom control bar | `playerUI.ControlBarUIMC` | `symbol4131` | `(600, 640)` |
| Minimap HUD | `playerUI.SmallMapUIMC` | `symbol1825` | `(1200, 0)` |
| Quest tracker | `playerUI.TaskTracerUIMC` | `symbol4135` | `(960, 230)` |

The positions are supported by the recovered ActionScript:

- `ControlBarUI`: `x = stageWidth / 2`, `y = stageHeight`.
- `SmallMapUI`: `x = stageWidth`.
- `GameInfoUI`: `y = stageHeight - 70`.
- `TaskTracerUI`: `x = stageWidth - titleBox.width`, `y = 230`; the title box resolves to 240 px.
- `PlayerInfoUI` remains at the display-list origin.

## Important asset boundary

`img.rar` contains minimap/reference images used inside the minimap system. It is **not** the source of the full playable world maps.

The complete playable map resources remain in the user's local multi-GB resource directory and will be integrated by the map importer/streamer later.

Therefore the reconstruction pipeline treats these as separate systems:

```text
SmallMapUIMC          -> minimap frame, buttons, markers and clipping viewport
img.rar               -> minimap image content
local world map data  -> actual playable map rendered by the world renderer
```

## Why whole exported PNGs are not the final UI

The recovered sprite PNG previews flatten all display-list children and may show children that ActionScript hides at runtime (for example optional icon groups or chat face panels). They are useful as visual references, but Eudoria reconstructs the actual child hierarchy from SWF placement tags.

Use `tools/swf_ui_payload/swf_ui_payload.py` to generate the direct first-frame display list for the five main HUD roots. That generated payload is the source for the native UI implementation.
