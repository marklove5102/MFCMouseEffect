# Phase 56zq - macOS Hover Plan Header Boundary and ObjC++ Surface Prune

## What Changed
- Split hover core responsibilities to tighten C++/ObjC++ boundary:
  - `MacosHoverPulseOverlayRendererCore.Plan.cpp` now keeps only render-plan construction (`BuildHoverPulseRenderPlan`).
  - `ConfigureHoverRingLayer(...)` moved to `MacosHoverPulseOverlayRendererCore.Layers.cpp` (ObjC++ leaf path).
- Hardened `MacosHoverPulseOverlayRendererCore.Internal.h` to support C++-only compile for plan translation unit:
  - Added non-ObjC type aliases for `NSView`/`CAShapeLayer`/`NSRect`.
  - Replaced `NSZeroRect` init with `CGRectZero`.
- Removed `MacosHoverPulseOverlayRendererCore.Plan.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.

## Why
- Plan file is pure compute/geometry assembly and should not remain in ObjC++ mode.
- Keeping ObjC message-send code only in layers/runtime files shrinks residual Objective-C surface without behavior change.

## Validation
- Syntax probe:
  - `MacosHoverPulseOverlayRendererCore.Plan.cpp` passes `-x c++ -fsyntax-only`.
- Build:
  - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
  - `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- Regression:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
  - `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

## Result
- All gates passed.
- ObjC++ allowlist reduced by one additional file with unchanged hover behavior.
