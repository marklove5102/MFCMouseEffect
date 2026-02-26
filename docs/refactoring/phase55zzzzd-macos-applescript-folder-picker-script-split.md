# Phase 55zzzzd - macOS AppleScript Folder Picker Script Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.Script.mm` mixed:
  - entry-thread dispatch
  - string/trim/path helpers
  - AppleScript source generation
  - AppleScript execution/error parsing
- One-file mixing increased maintenance risk for picker fixes.

## What Changed
- Added shared script helper contract:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.ScriptHelpers.h`
- Split helper domains:
  - string/path utilities: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.StringUtils.mm`
  - source build + execute: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.Execution.mm`
- Kept entry function thin:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.Script.mm`
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `WASM` plugin import path (folder picker for plugin folder selection).
- Not part of: native effects visuals, input indicator, gesture mapping runtime.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- Behavior preserved; this is responsibility split with unchanged picker contract.
