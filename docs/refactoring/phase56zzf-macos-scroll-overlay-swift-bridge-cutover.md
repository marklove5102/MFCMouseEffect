# Phase 56zzf - macOS Scroll Overlay Swift Bridge Cutover

## Scope
- Capability: `effects` (scroll category).
- Goal: migrate scroll overlay rendering leaf from ObjC++ to Swift bridge and keep ObjC++ allowlist shrinking.

## Decision
- Add dedicated Swift bridge for scroll overlay rendering:
  - body capsule
  - direction arrow
  - helix and twinkle decorations
  - scale/fade animation
- Keep `MacosScrollPulseOverlayRendererCore.Layers.cpp` as pure C++ orchestration:
  - plan->bridge call
  - window registry/show lifecycle
  - delayed close scheduling
- Remove scroll layers file from ObjC++ allowlist.

## Code Changes
1. Added bridge files:
- `MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayBridge.swift`
- `MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlaySwiftBridge.h`

2. Refactored scroll layers entry:
- `MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Layers.cpp`
  - removed direct AppKit/CALayer and ObjC block logic.
  - delegates render path to Swift bridge.
  - uses `dispatch_after_f` close callback context for delayed cleanup.

3. Internal contract cleanup:
- `MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Internal.h`
  - removed obsolete ObjC-only helper declarations.

4. Build wiring:
- `MFCMouseEffect/Platform/macos/CMakeLists.txt`
  - added Swift compile/object wiring for scroll bridge.
  - removed `MacosScrollPulseOverlayRendererCore.Layers.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- Scroll overlay rendering leaf is Swift-owned.
- ObjC++ allowlist reduced from `3` to `2`.

