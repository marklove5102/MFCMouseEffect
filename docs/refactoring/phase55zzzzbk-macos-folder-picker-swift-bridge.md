# Phase 55zzzzbk - macOS Folder Picker Swift Bridge (Swift-first with fallback)

## Background
- macOS folder selection for WASM import was previously routed through Objective-C++ picker implementations.
- Current direction is Swift-first for new macOS closure work, while keeping risk controlled.

## Decision
- Keep `platform::PickFolder(...)` C++ interface unchanged.
- Add Swift folder-picker bridge and make macOS path use:
  - `Swift bridge first`
  - `existing MacosNativeFolderPicker fallback`
- This preserves behavior while starting migration away from Objective-C++ as the primary path.

## Implementation
1. New Swift bridge
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosNativeFolderPickerBridge.swift`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosNativeFolderPickerSwiftBridge.h`
- Exports C ABI:
  - `mfx_macos_pick_folder_v1(...) -> int32_t`
  - Return contract:
    - `>0`: success
    - `0`: cancelled
    - `<0`: failed

2. C++ route integration
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/PlatformNativeFolderPicker.cpp`
- Behavior:
  - On macOS, call Swift bridge first.
  - If bridge returns `ok/cancelled`, return directly.
  - On bridge failure, fallback to existing `MacosNativeFolderPicker` path.
  - Preserve `error` aggregation semantics (`fallback_from_swift_bridge: ...`).

3. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Added Swift object build for picker bridge via `swiftc` custom command (same style as notification Swift bridge), compatible with current generator flow.

## Validation
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto --skip-core-smoke --skip-core-automation --skip-macos-automation-injection-selfcheck --skip-macos-wasm-selfcheck --skip-linux-gate --skip-automation-test`

## Impact
- Capability: `WASM（插件目录导入前置能力）`
- User-visible API contract unchanged.
- Migration path becomes Swift-first without forcing a risky big-bang replace.
