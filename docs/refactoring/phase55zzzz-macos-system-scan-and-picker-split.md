# Phase 55zzzz - macOS system scan and picker split

## Summary
- Capability: `automation mapping` app-catalog scan path + `wasm` plugin folder pick path.
- This slice splits macOS system workflows into entry and helper units.

## Changes
1. Application catalog scan workflow split
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.Helpers.mm`
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.mm`
- Result:
  - workflow file keeps root iteration and final sorting;
  - helper file owns bundle-name/display-name resolve and per-root scan internals.

2. AppleScript folder picker split
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.Script.mm`
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosAppleScriptFolderPicker.mm`
- Result:
  - public entry keeps stable API;
  - AppleScript source/execute/parse details move to dedicated script unit.

3. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Validation
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/docs/doc-hygiene-check.sh --strict
```

## Compatibility
- No behavior/API contract changes intended for app scan and folder pick flows.
