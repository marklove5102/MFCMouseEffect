# Phase 56zj - macOS Hold Core ObjC++ Surface Prune

## What Changed
- Removed `MacosHoldPulseOverlayRendererCore.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.
- Replaced Objective-C null sentinel usage in that file:
  - `nil` -> `nullptr` for `window/ring/accent` lifecycle reset checks.

## Why
- After previous header boundary cleanup, this file no longer requires Objective-C syntax and compiles as pure C++.
- Keeping it in ObjC++ allowlist would overstate residual ObjC++ surface area.

## Validation
- Build:
  - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
  - `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- Regression:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
  - `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

## Result
- All build and regression gates passed.
- ObjC++ allowlist shrank further without behavior changes.
