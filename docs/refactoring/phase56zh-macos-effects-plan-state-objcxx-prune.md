# Phase 56zh - macOS Effects Plan/State ObjC++ Surface Prune

## What Changed
- Converted shared effects headers to be C++-compatible by replacing unconditional AppKit imports with:
  - CoreGraphics/CoreFoundation base types
  - ObjC forward declarations for ObjC++ mode
  - opaque pointer aliases for pure C++ mode
- Removed `4` files from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`:
  - `MacosClickPulseOverlayRendererCore.Plan.cpp`
  - `MacosTrailPulseOverlayRendererCore.Plan.cpp`
  - `MacosScrollPulseOverlayRendererCore.Plan.cpp`
  - `MacosHoldPulseOverlayRendererCore.State.cpp`

## Refactor Boundary
- Updated headers:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosOverlayRenderSupport.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRendererCore.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererSupport.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayStyle.h`
- Added explicit AppKit/QuartzCore imports back into ObjC++ implementation files that require full interfaces:
  - `MacosClickPulseOverlayRendererCore.Layers.cpp`
  - `MacosTrailPulseOverlayRendererCore.Layers.cpp`
  - `MacosScrollPulseOverlayRendererCore.Layers.cpp`
  - `MacosHoldPulseOverlayStyle.cpp`
  - `MacosHoldPulseOverlayStyle.Accent.cpp`

## Safety Notes
- This change does not alter effect command semantics or renderer behavior.
- It only narrows compile-mode scope by decoupling pure planning/state files from transitive AppKit dependencies.

## Validation
- Build:
  - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
  - `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- Regression:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
  - `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

## Result
- All validation commands passed.
- ObjC++ allowlist is further reduced while preserving macOS behavior and existing cross-platform contracts.
