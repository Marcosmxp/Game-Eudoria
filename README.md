# Eudoria

Eudoria is a native Windows reconstruction project based on recovered legacy Crystal Saga client resources. The target is a faithful offline desktop game while replacing Flash/SWF runtime dependencies and online server transport with native rendering and local game systems.

## Current phase

**Phase 0 - Foundation and legacy catalogue.**

Implemented so far:

- native Win32 application bootstrap
- Direct3D 11 device/swap chain
- windowed mode + F11 borderless fullscreen
- 1200x640 legacy UI reference space
- initial HUD payload and legacy SymbolClass mapping
- local scanner for the multi-GB resource dump
- reconstruction architecture and phased plan

## Technical direction

- C++23
- Win32
- Direct3D 11
- DirectWrite (planned)
- WIC (planned)
- XAudio2 (planned)
- CMake
- no browser runtime
- no Flash Player runtime
- no game engine

## Build

Requirements:

- Windows 10/11
- Visual Studio 2022 with Desktop development with C++
- CMake 3.24+

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

Executable:

```text
build/Debug/Eudoria.exe
```

Controls in the bootstrap runtime:

- `F11`: toggle borderless fullscreen
- `Esc`: close application

## Legacy asset policy

The original multi-GB dump stays local and is ignored by Git. Do not commit `resources_by_type`, extracted legacy assets, RAR/ZIP dumps, generated saves, or build output.

Use the scanner instead:

```powershell
py tools/legacy_scanner/legacy_scanner.py `
  "C:\Users\Marcos\Downloads\CrystalSaga\resources_by_type" `
  --output generated/legacy
```

See `docs/reconstruction-plan.md` for the current implementation order.
