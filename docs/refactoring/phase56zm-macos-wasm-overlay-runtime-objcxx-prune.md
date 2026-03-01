# Phase 56zm - macOS WASM Overlay Runtime ObjC++ Surface Prune

## What Changed
- Refactored `Platform/macos/Wasm/MacosWasmOverlayRuntime.cpp` to pure C++ runtime dispatch path:
  - Replaced `NSThread` + `dispatch_sync/dispatch_async` block literals with `pthread_main_np` + `dispatch_sync_f/dispatch_async_f` callback dispatch.
  - Replaced direct `NSWindow` Objective-C message-send close path with `macos_overlay_support::ReleaseOverlayWindow(...)`.
- Removed `MacosWasmOverlayRuntime.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST` in `Platform/macos/CMakeLists.txt`.

## Why
- Runtime host scheduling logic does not need Objective-C syntax when callback-based GCD APIs are used.
- Window-release behavior is already centralized in overlay support; using that path reduces duplicate lifecycle handling.

## Validation
- Build:
  - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
  - `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- Regression:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
  - `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

## Result
- All gates passed.
- ObjC++ allowlist shrank again without changing WASM runtime API contracts.
