# Eudoria UI reconstruction status

Current native HUD migration order:

1. `playerUI.PlayerInfoUIMC` - active; payload-derived skin and animated HP/MP frames.
2. `playerUI.ControlBarUIMC` - active; payload-derived layout, button states and input wiring.
3. `playerUI.SmallMapUIMC` - active chrome; `img.rar` minimap image can be injected into the payload-defined viewport.
4. `playerUI.GameInfoUIMC` - payload-derived chrome active; dynamic chat text/tabs pending migration.
5. `playerUI.TaskTracerUIMC` - payload-derived chrome active; dynamic quest rows pending migration.

The legacy payload is authoritative. Screenshots are comparison references only and are never runtime UI assets.
