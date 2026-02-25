# Phase 55zzf: macOS Click Pulse Overlay Internals Split

## Capability
- Effect

## Why
- `MacosClickPulseEffect.mm` mixed effect lifecycle entry with rendering/style/window-lifecycle details.
- This made click-effect evolution and review harder than scroll effect path, which is already split by responsibility.

## Scope
- Keep click pulse visual behavior unchanged.
- Align click-effect structure with existing scroll-effect split model.
- Keep `MacosClickPulseEffect` as lifecycle + event-entry facade only.

## Code Changes

### 1) New renderer module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRenderer.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRenderer.mm`
- Owns:
  - main-thread sync/async wrapper
  - overlay window/layer composition
  - pulse animation and delayed close scheduling

### 2) New style module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayStyle.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayStyle.mm`
- Owns:
  - button-based stroke/fill color mapping

### 3) New window-registry module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseWindowRegistry.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseWindowRegistry.mm`
- Owns:
  - transient overlay window handle set
  - register/take semantics
  - close-all drain for shutdown

### 4) Effect facade simplification
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseEffect.mm`
- Keeps:
  - initialization state and lifecycle
  - click event entry + coordinate conversion
  - call-through to renderer module

### 5) Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- No API/schema contract changes.
- Click pulse timing, color semantics, and visibility behavior are unchanged.
