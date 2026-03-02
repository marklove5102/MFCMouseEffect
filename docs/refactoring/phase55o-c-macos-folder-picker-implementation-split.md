# Phase 55o-c: macOS Folder Picker Implementation Split

## Issue Classification
- Verdict: `Architecture maintainability improvement`.
- Problem: `MacosNativeFolderPicker.mm` accumulated AppleScript path + OpenPanel fallback + coordinator logic in one file (~300+ lines), increasing coupling and review cost.

## Goal
1. Split picker logic by responsibility without changing behavior.
2. Keep endpoint contract unchanged for `/api/wasm/import-from-folder-dialog`.
3. Reduce single-file complexity for long-term cross-platform maintenance.

## Implementation
- Coordinator file remains:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosNativeFolderPicker.mm`
  - now only orchestrates:
    - `PickFolderViaAppleScript(...)`
    - fallback `PickFolderViaOpenPanel(...)`
    - fallback error composition
- New AppleScript implementation module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.mm`
- New OpenPanel fallback module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosOpenPanelFolderPicker.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosOpenPanelFolderPicker.mm`
- Build wiring update:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
  - includes both new `.mm` files for macOS core runtime build.

## Behavior Contract
- No intentional behavior change.
- Import dialog still follows:
  1. in-process AppleScript choose-folder path first,
  2. OpenPanel fallback only on AppleScript non-cancel failure,
  3. identical `ok/cancelled/error/folderPath` contract semantics.

## Validation
- `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build`
- `./tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh --skip-build`
