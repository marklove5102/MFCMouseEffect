# Phase 56zz - macOS WASM Text Overlay Swift Bridge Cutover

## Scope
- Capability: `wasm` (text overlay render path).
- Goal: move macOS WASM text overlay panel/UI path from ObjC++ to Swift bridge and shrink ObjC++ allowlist.

## Decision
- Introduce a dedicated Swift bridge for WASM text overlay lifecycle (`create/show/release`).
- Keep `MacosWasmTextOverlay.cpp` as pure C++ orchestration (layout + admission + delayed close scheduling).
- Remove legacy style module build path (`MacosWasmTextOverlay.Style.cpp`) after Swift cutover.

## Code Changes
1. Added Swift bridge and C ABI:
- `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlayBridge.swift`
- `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlaySwiftBridge.h`

2. Refactored runtime entry:
- `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.cpp`
  - Removed AppKit ObjC++ panel construction.
  - Delegates panel lifecycle to Swift bridge.
  - Uses C++ `dispatch_after_f` close context for delayed release.

3. Simplified internal contract:
- `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.Internal.h`
  - Kept only layout contract.
  - Removed ObjC panel/label helper declarations.

4. Removed replaced file:
- Deleted `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.Style.cpp`.

5. Updated build wiring:
- `MFCMouseEffect/Platform/macos/CMakeLists.txt`
  - Added Swift compile step for `MacosWasmTextOverlayBridge.swift`.
  - Added Swift object to `mfx_shell_macos` target.
  - Removed `MacosWasmTextOverlay.Style.cpp` from source list.
  - Removed `MacosWasmTextOverlay.cpp` and style file from ObjC++ allowlist.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-wasm-regression-suite.sh --platform auto`

All commands passed on macOS.

## Result
- WASM text overlay runtime UI path is Swift-owned.
- ObjC++ allowlist reduced from `9` to `7`.
