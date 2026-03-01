# Phase 56zg - macOS ObjC++ Allowlist Compiler-Probe Prune

## What Changed
- Shrunk `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST` in `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt` by removing `26` files that compile cleanly as pure C++.
- Kept ObjC++ compilation only for files that still require AppKit/Objective-C syntax through direct or transitive headers.

## Why
- Previous regex/text-only heuristics can miss transitive AppKit dependencies in included headers.
- This pass used compiler-level probing (`-x c++ -fsyntax-only`) per allowlist entry to avoid false pruning.

## Pruned Buckets
- Effects runtime wrappers:
  - `MacosClickPulseEffect.cpp`
  - `MacosTrailPulseEffect.cpp`
  - `MacosScrollPulseEffect.cpp`
  - `MacosHoverPulseEffect.cpp`
  - `MacosHoldPulseEffect.cpp`
- Input indicator overlay wrappers:
  - `MacosInputIndicatorOverlay*.cpp` (8 files)
- WASM command/state wrappers:
  - `MacosWasmCommandRender*.cpp` and `MacosWasmCommandRenderer.cpp`
  - `MacosWasmImageOverlayRenderer.cpp`
  - `MacosWasmImageOverlayRendererCore.cpp`
  - `MacosWasmOverlayPolicy.cpp`
  - `MacosWasmOverlayState*.cpp` (4 files)
  - `MacosWasmTransientOverlay.cpp`

## Validation
- Build:
  - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
  - `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- Regression:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
  - `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

## Result
- All build and regression commands passed.
- ObjC++ surface continues to contract without changing Windows behavior or POSIX runtime contracts.
