# Legacy stage scaling contract

The original Crystal Saga client configures Flash with:

```actionscript
stage.align = StageAlign.TOP_LEFT;
stage.scaleMode = StageScaleMode.NO_SCALE;
```

Source: `Crystal Saga.rar -> scripts/SomcGame.as -> initStage()`.

Eudoria therefore keeps legacy HUD artwork at native pixel size. Window/fullscreen resize changes `stageWidth`/`stageHeight` and the HUD roots are re-anchored to those new dimensions; the HUD itself is not uniformly stretched.

This is important for two reasons:

1. it preserves the original pixel/raster quality instead of enlarging legacy PNG exports;
2. it reproduces the original resize behavior used by `ControlBarUI`, `SmallMapUI`, `TaskTracerUI`, and the other stage-edge HUD components.

`LegacyUiTransform::scale()` intentionally returns `1.0f`. Any future world/camera scaling must remain separate from the legacy HUD stage contract.
