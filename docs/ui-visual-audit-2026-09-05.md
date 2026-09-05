# UI visual audit — 2026-09-05

The Windows screenshots exposed two separate issues that must not be treated as one problem.

## 1. Fullscreen blur

The legacy client does **not** scale the Flash stage. `SomcGame.initStage()` explicitly uses `StageScaleMode.NO_SCALE` and `StageAlign.TOP_LEFT`.

Eudoria was incorrectly multiplying the HUD by `min(viewportWidth / 1200, viewportHeight / 640)`. That enlarged rasterized legacy assets in fullscreen and made icons/text softer.

Correction: keep legacy HUD pixels at 1:1 and only re-anchor roots against the actual window `stageWidth` / `stageHeight`.

## 2. SmallMap visual spill

`SmallMapUIMC` character 1825 contains the minimap chrome plus `totalIcon` (character 1815), which extends hundreds of pixels to the left. Rendering the whole FFDec reference PNG as one runtime sprite forces all first-frame children to appear together.

The native runtime must render the minimap chrome independently from the expandable `totalIcon` feature panel. Until character 1815 is migrated as an interactive component, the runtime uses only the payload-derived minimap chrome region; the complete reference remains available through F2 for comparison.

The crop geometry is derived from the SWF display list, not from screenshots:

- SmallMap root: `(stageWidth, 0)`
- background character 1632 bounds: `x=-181..0`, `y=0..192`
- FFDec SmallMap reference raster origin: `(-981, -8.5)` relative to the instance
- therefore the 181x192 chrome region begins at source `(800, 8.5)`
- minimap viewport remains `(-141, 34, 125, 130)`
