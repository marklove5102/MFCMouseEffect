# Phase 56zi - macOS WASM Plan/Layout ObjC++ Surface Prune

## What Changed
- Made WASM shared headers C++-compatible by replacing unconditional AppKit imports with:
  - CoreGraphics base types
  - ObjC forward declarations in ObjC++ mode
  - opaque pointer aliases in C++ mode
- Removed `2` files from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`:
  - `MacosWasmImageOverlayRendererCore.Plan.cpp`
  - `MacosWasmTextOverlay.Layout.cpp`

## Refactor Boundary
- Updated headers:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmImageOverlayRendererSupport.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayRenderMath.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.Internal.h`
- Updated implementation imports where full Objective-C interfaces are required:
  - `MacosWasmImageOverlayRendererSupport.cpp`
  - `MacosWasmOverlayRenderMath.cpp`
  - `MacosWasmTextOverlay.Style.cpp`
- Minor geometry normalization:
  - `MacosWasmTextOverlay.Layout.cpp` now uses `CGRectMake` for C++ compatibility.

## Safety Notes
- No runtime behavior or route contract was changed.
- This pass only narrows ObjC++ compile scope for WASM planning/layout files.

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
- ObjC++ allowlist continued shrinking with no observed macOS/Windows/Linux contract regression.
