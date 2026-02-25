# Phase 55zy: macOS Scroll Pulse Overlay Renderer Split

## Capability
- Effect

## Why
- `MacosScrollPulseEffect.mm` mixed effect lifecycle and full overlay window rendering internals.
- Window registry, animation geometry, color/path composition, and scheduling were tightly coupled with event entry.

## Scope
- Keep scroll visual behavior unchanged.
- Split overlay rendering internals into dedicated renderer module.
- Keep effect class focused on lifecycle and dispatch entry.

## Code Changes

### 1) New overlay renderer module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRenderer.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRenderer.mm`
- Owns:
  - window registry lifecycle (`register/take/close-all`)
  - main-thread dispatch helpers
  - direction arrow/body path + color composition
  - animation timing and delayed close scheduling

### 2) Effect facade simplification
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseEffect.mm`
- Keeps:
  - `Initialize/Shutdown`
  - `OnScroll` entry + coordinate conversion + renderer invocation

### 3) Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Contract Impact
- No API/schema change.
- Scroll pulse visual and timing behavior unchanged.
