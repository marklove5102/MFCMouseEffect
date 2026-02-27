# Phase 55zzzzbl - macOS Settings Launcher Swift Bridge

## Background
- macOS settings URL launching previously always used POSIX command spawn (`open <url>`).
- We need Swift-first macOS boundary migration while preserving existing regression contracts.

## Decision
- Add Swift launcher bridge (`NSWorkspace.open`) for normal runtime.
- Keep existing POSIX launcher path as fallback.
- Keep capture-mode behavior unchanged (`MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE`) so existing core/scaffold contract probes remain stable.

## Implementation
1. New Swift bridge
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosSettingsLauncherBridge.swift`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosSettingsLauncherSwiftBridge.h`
- Exported C ABI:
  - `mfx_macos_open_settings_url(const char* urlUtf8) -> bool`

2. C++ launcher integration
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosSettingsLauncher.cpp`
- Behavior:
  - capture mode (`MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE` set): keep POSIX route unchanged
  - normal mode: validate input -> try Swift bridge -> fallback to POSIX `open` command

3. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Added Swift object build for settings launcher bridge via existing `swiftc` custom-command flow.

## Validation
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto --skip-core-smoke --skip-core-automation --skip-macos-automation-injection-selfcheck --skip-macos-wasm-selfcheck --skip-linux-gate --skip-automation-test`

## Impact
- Capability: `键鼠指示/手势映射（设置页打开链路）`
- User-visible behavior remains the same.
- Test capture contracts remain unchanged.
