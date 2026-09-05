# Eudoria

Eudoria is a native desktop reconstruction project inspired by the legacy Crystal Saga client. The project targets a faithful offline single-player experience while replacing Flash/SWF runtime dependencies with a native Windows renderer and local game systems.

## Current phase

Phase 0 - legacy asset cataloguing, native runtime bootstrap, and HUD reconstruction groundwork.

## Technical direction

- C++23
- Win32
- Direct3D 11
- DirectWrite
- WIC
- XAudio2
- CMake
- Native Windows executable
- No browser runtime
- No Flash Player runtime
- No game engine

## Legacy reference resolution

The original client used a 1200x640 logical stage. Eudoria keeps this as a legacy UI coordinate space for pixel-accurate reconstruction while allowing the world viewport to expand on modern fullscreen resolutions.

## Repository policy

Large original resource dumps are kept outside Git. Generated manifests, schemas, tooling, documentation, and Eudoria-owned assets belong in this repository.
