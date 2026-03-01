# Phase56ze: macOS Effects Compute/Registry ObjC++ Prune

## Background
- A subset of effects modules are pure compute/registry code paths and do not use AppKit/ObjC runtime.
- They were still in `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST` from historical broad matching.

## What Changed
1. Shrunk ObjC++ allowlist for pure C++ modules
- Updated:
  - `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Removed from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`:
  - `MacosEffectCreatorRegistry.cpp`
  - `MacosEffectCreatorRegistry.Table.cpp`
  - `MacosEffectComputeProfileAdapter.cpp`
  - `MacosEffectRenderProfile.cpp`
  - `MacosEffectRenderProfile.ClickTrail.cpp`
  - `MacosEffectRenderProfile.ScrollHoldHover.cpp`

## Validation
```bash
cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8
cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto
./tools/platform/regression/run-macos-objcxx-surface-regression.sh
```

## Notes
- No behavior or API contract change.
- This phase only tightens compilation surface to reflect actual runtime dependency boundaries.
