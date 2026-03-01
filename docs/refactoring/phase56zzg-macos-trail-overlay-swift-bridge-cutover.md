# Phase 56zzg - macOS Trail Overlay Swift Bridge Cutover

## Scope
- Capability: `effects` (trail category).
- Goal: migrate trail overlay rendering leaf from ObjC++ to Swift bridge and continue ObjC++ surface reduction.

## Decision
- Add dedicated Swift bridge for trail overlay rendering:
  - line/streamer/electric/tubes/particle/meteor path logic
  - glow and meteor outer-halo composition
  - scale/fade animation
- Keep `MacosTrailPulseOverlayRendererCore.Layers.cpp` as pure C++ orchestration:
  - plan->bridge call
  - window registry/show lifecycle
  - delayed close scheduling
- Remove trail layers file from ObjC++ allowlist.

## Code Changes
1. Added bridge files:
- `MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayBridge.swift`
- `MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlaySwiftBridge.h`

2. Refactored trail layers entry:
- `MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Layers.cpp`
  - removed direct AppKit/CALayer/ObjC block logic.
  - delegates render path to Swift bridge.
  - uses `dispatch_after_f` close callback context.

3. Internal contract cleanup:
- `MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Internal.h`
  - removed obsolete ObjC-only helper declarations.

4. Build wiring:
- `MFCMouseEffect/Platform/macos/CMakeLists.txt`
  - added Swift compile/object wiring for trail bridge.
  - removed `MacosTrailPulseOverlayRendererCore.Layers.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- Trail overlay rendering leaf is Swift-owned.
- ObjC++ allowlist reduced from `2` to `1` (only hold-start leaf remains).

