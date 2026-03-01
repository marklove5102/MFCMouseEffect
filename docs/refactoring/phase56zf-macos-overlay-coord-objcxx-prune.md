# Phase56zf: macOS Overlay Coord ObjC++ Prune

## Background
- `MacosOverlayCoordSpaceConversion.cpp` and `MacosOverlayCoordSpaceService.cpp` were already Swift-bridge backed and contain pure C++ logic.
- They still remained in macOS ObjC++ allowlist due historical carry-over.

## What Changed
1. ObjC++ allowlist reduced for overlay coordinate modules
- Updated:
  - `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Removed from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`:
  - `MacosOverlayCoordSpaceConversion.cpp`
  - `MacosOverlayCoordSpaceService.cpp`

## Validation
```bash
cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8
cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto
./tools/platform/regression/run-macos-objcxx-surface-regression.sh
```

## Notes
- Structural compile-surface cleanup only; no behavior/interface change.
