# Phase 55m: macOS Native Folder Picker for WASM Import Dialog

## Issue Classification
- Verdict: `Bug/regression risk`.
- Symptom: macOS core lane had WASM runtime/render and catalog/transfer APIs, but `POST /api/wasm/import-from-folder-dialog` still returned platform-unsupported.
- Impact: shared Svelte WebUI had an uneven plugin-import path across Windows/macOS.

## Goal
1. Enable native folder picker on macOS for WASM plugin folder import.
2. Keep Windows behavior unchanged.
3. Keep Linux behavior unchanged (still explicit unsupported).

## Implementation
- Added macOS native folder picker service:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosNativeFolderPicker.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosNativeFolderPicker.mm`
  - current implementation uses in-process AppleScript chooser first, with `NSOpenPanel` fallback on non-cancel failure.
  - detailed modules:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.mm`
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosOpenPanelFolderPicker.mm`
- Routed platform facade to macOS implementation:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/PlatformNativeFolderPicker.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/PlatformNativeFolderPicker.h`
- Wired source into macOS core runtime build:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Removed Windows-only route guard for folder-dialog import endpoint:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- Added test-friendly probe branch (no modal dialog):
  - `POST /api/wasm/import-from-folder-dialog` with `{\"probe_only\": true}`
  - returns support capability without opening native dialog.
- Extended core automation HTTP contracts for probe mode:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`

## Extra Cross-Platform Hardening
- While enabling POSIX plugin transfer/catalog builds, two Windows-only assumptions were removed:
  - `_wcsicmp` usage replaced by portable case-insensitive comparison:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginCatalog.cpp`
  - `SYSTEMTIME/GetLocalTime` replaced by portable time formatting:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginTransferService.cpp`

## Behavior Change
- On macOS core lane:
  - `POST /api/wasm/import-from-folder-dialog` now runs native folder picker flow instead of immediate unsupported response.
  - Folder picker now uses in-process AppleScript chooser path by default; OpenPanel stays fallback-only.
- On Linux:
  - endpoint remains explicit unsupported.
- For tests/CI:
  - `probe_only=true` allows non-interactive capability assertion on all platforms.

## Stability Follow-up (2026-02-24)
- Issue classification: `Bug/regression`.
- Symptom: on macOS, `Import Plugin Folder` panel could close immediately after arbitrary click in some tray/background activation states.
- Root cause: modal panel opened without guaranteed foreground interaction context under accessory-policy shell process.
- Fix:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosNativeFolderPicker.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosOpenPanelFolderPicker.mm`
  - keep scoped modal activation guard for `NSOpenPanel` fallback.
  - historical step added external `osascript` chooser first; current implementation has been upgraded to in-process `NSAppleScript` chooser with the same fallback contract.
  - keep existing endpoint contract (`ok/cancelled/error/selected_folder_path/...`) unchanged.
- Validation:
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build` (pass)

## UX Follow-up (Dock icon during import)
- Issue classification: `Bug/regression`.
- Symptom: clicking `Import Plugin Folder` could show transient `exec` icon in Dock.
- Root cause:
  - external process chooser invocation (`popen + /usr/bin/osascript`) can surface a transient non-bundle process icon (`exec`) in Dock.
  - fallback path with activation-policy promotion could further amplify Dock visibility side effects.
- Fix:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosNativeFolderPicker.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosOpenPanelFolderPicker.mm`
  - keep AppleScript default-location alias coercion:
    - `default location ((POSIX file "...") as alias)`
  - switch chooser execution from external `popen` command to in-process `NSAppleScript` execution.
  - keep `NSOpenPanel` as fallback only, and remove fallback activation-policy promotion (retain foreground activation only).
- Validation:
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build` (pass)

## Validation
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto` (pass)

## Next
- Phase 55n manual acceptance:
  - verify end-to-end plugin folder import on macOS settings page (`Import Plugin Folder`).
