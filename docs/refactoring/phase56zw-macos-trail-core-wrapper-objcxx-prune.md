# Phase 56zw - macOS Trail Core Wrapper ObjC++ Prune

## Scope
- Capability: `effects` (trail category).
- Goal: reduce ObjC++ surface by keeping trail core wrapper file C++-only.

## Decision
- Keep `MacosTrailPulseOverlayRendererCore.cpp` as pure wrapper/orchestration.
- Move AppKit/CALayer command-path rendering into `MacosTrailPulseOverlayRendererCore.Layers.cpp`.
- Remove `MacosTrailPulseOverlayRendererCore.cpp` from ObjC++ allowlist.

## Code Changes
1. `MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.cpp`
- Removed AppKit/QuartzCore/dispatch and window lifecycle code.
- Kept compute-wrapper overload:
  - `ShowTrailPulseOverlayOnMain(overlayPt, deltaX, deltaY, effectType, themeName, profile)`.

2. `MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Layers.cpp`
- Added command-path renderer implementation:
  - `ShowTrailPulseOverlayOnMain(const TrailEffectRenderCommand&, const std::string&)`.
- Added required includes:
  - `MacosTrailPulseOverlayRendererCore.h`
  - `MacosTrailPulseWindowRegistry.h`
  - `dispatch/dispatch.h`

3. `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Removed `MacosTrailPulseOverlayRendererCore.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- ObjC++ allowlist reduced from `12` to `11`.
- Trail core wrapper file is now C++-only; ObjC++ remains in layers leaf.
