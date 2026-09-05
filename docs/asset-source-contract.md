# Eudoria UI source-of-truth policy

- `Crystal Saga.rar`: authoritative HUD/UI source (SWF display lists, ActionScript, sprites, shapes, buttons, text fields, states, animation frames).
- `img.rar`: minimap/reference images such as `p<mapId>.jpg`; never the playable world map source.
- local multi-GB assets: playable world maps and other heavy world resources.
- `txt.rar` / recovered configs: gameplay and configuration data.
- screenshots: visual comparison only.

If a screenshot conflicts with the payload, the payload wins. Unknown values stay marked as pending migration instead of being permanently guessed.
