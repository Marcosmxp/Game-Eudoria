# Eudoria Architecture

## Objective

Rebuild the legacy Crystal Saga client as a native, offline desktop game while preserving the original presentation, UI behaviour, map data relationships, animations, and gameplay rules as reference material.

The final runtime does not execute Flash content. SWF, JSON/TXT configuration, map tiles, minimaps, and decompiled ActionScript are import/reference sources used to produce native Eudoria data.

## Runtime stack

- C++23
- Win32 window/input lifecycle
- Direct3D 11 rendering
- DirectWrite (planned text renderer)
- WIC (planned image decoder)
- XAudio2 (planned audio)
- CMake

## Runtime layers

1. Platform
   - window
   - input
   - fullscreen
   - timing
2. Renderer
   - textures
   - sprite batching
   - map tiles
   - UI composition
   - particles/effects
3. Core
   - resource IDs
   - event bus
   - game clock
   - serialization
4. Game systems
   - world/maps
   - player
   - combat
   - NPC/monster AI
   - inventory
   - quests
   - pets/mounts
5. UI
   - legacy-compatible display tree
   - windows
   - HUD
   - tooltips
   - drag/drop

## Legacy coordinate model

The captured client uses a 1200x640 logical game stage. Eudoria preserves it as a reference coordinate system.

Legacy comparison mode renders against that exact logical stage. Modern fullscreen mode keeps HUD elements anchored while allowing the world camera to occupy the additional viewport.

## Flash compatibility strategy

We do not implement a Flash runtime. We implement only the presentation concepts needed by the recovered client data:

- hierarchical display nodes
- transform: position, scale, rotation, alpha, pivot
- clipping/masks
- blend mode
- color transform
- bitmap/sprite nodes
- text nodes
- buttons and interaction states
- frame animation
- nine-slice window panels
- z ordering

## Offline conversion

Legacy network commands become local game commands/events.

Example:

`UI -> network request -> server -> notify -> state`

becomes:

`UI/Input -> local command -> game system -> state -> event -> world/UI`

The decompiled ActionScript remains valuable as behavioural documentation, but server transport is not reproduced.
