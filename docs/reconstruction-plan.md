# Reconstruction Plan

## Phase 0 - Foundation (current)

- [x] Initialize repository.
- [x] Create `development` branch.
- [x] Bootstrap native Win32 + Direct3D 11 executable.
- [x] Define 1200x640 legacy UI coordinate space.
- [x] Add first HUD payload mapping.
- [x] Add local legacy resource scanner.
- [ ] Run scanner against the complete local 5.89 GB resource dump.
- [ ] Commit generated *summary* metadata that is safe/small enough to version.
- [ ] Correlate scanner output with `fetch_manifest.json`.

## Phase 1 - Legacy catalogue

- Resolve resource IDs from `data2.json` / fetch manifest.
- Index SWF headers, exports and SymbolClass names.
- Index minimap-to-config relationships (`p<ID>` <-> `<ID>`).
- Index map tile sets (`d_<map>/<row>_<column>`).
- Detect missing/orphan/duplicate resources.
- Build a canonical Eudoria asset registry while preserving `legacyId`.

## Phase 2 - UI renderer

- Orthographic UI projection.
- Texture loading and sprite batching.
- UI display tree.
- Alpha/blend/color transforms.
- Nine-slice panels.
- Bitmap/text/button/progress/scroll/list/grid/slot widgets.
- Clipping and masks.
- Tooltip and drag/drop layers.

Acceptance test: reconstruct the static main HUD in legacy 1200x640 mode and compare it against screenshots.

## Phase 3 - Main HUD

Priority components:

1. `playerUI.PlayerInfoUIMC`
2. `playerUI.SmallMapUIMC`
3. `playerUI.TaskTracerUIMC`
4. chat panel
5. `playerUI.ControlBarUIMC`

Then reconstruct window manager behaviour and representative windows:

- character
- inventory
- skill book
- quest log
- pet
- mount
- friends/guild
- achievements
- world map

## Phase 4 - World renderer

- tiled map streaming
- camera
- map coordinate conversion
- collision/path data
- entities and depth ordering
- minimap transform
- portals/spawns/NPC positions

## Phase 5 - Character and effects

- SWF-derived animation catalogue
- native animation state machine
- characters/monsters/NPCs
- equipment overlays
- wings/mounts/pets
- skill VFX
- particles and blend effects

## Phase 6 - Offline gameplay

Convert server-dependent flows into local systems:

- player stats/level progression
- inventory/equipment
- combat
- skills
- NPC AI
- quests
- drops
- pets
- mounts
- achievements
- local save files

## Fidelity rule

Do not redesign a recovered system merely because a modern alternative exists. During reconstruction, preserve layout, timings, proportions, terminology, states, and behaviours where the recovered resources establish them. Improvements are introduced only after a faithful baseline exists.
