# Phase 55zzb: macOS Tray Service Menu Factory Split

## Capability
- Shell/tray integration (supports settings entry for shared Svelte WebUI)

## Why
- `MacosTrayService.mm` mixed service lifecycle, Objective-C action bridge, menu building, and runtime helper logic.
- This increased coupling between tray orchestration and menu/runtime internals.

## Scope
- Keep tray behavior unchanged.
- Split menu/action bridge and runtime helpers into dedicated modules.
- Keep `MacosTrayService` focused on lifecycle/start-stop orchestration.

## Code Changes

### 1) Runtime helper module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosTrayRuntimeHelpers.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosTrayRuntimeHelpers.mm`
- Owns:
  - main-thread sync helper
  - utf8-to-NSString fallback conversion
  - test auto-trigger settings action scheduling

### 2) Menu factory module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosTrayMenuFactory.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosTrayMenuFactory.mm`
- Owns:
  - Objective-C action bridge class
  - status-item/menu creation and menu item binding
  - release/cleanup of tray objects

### 3) Tray service facade simplification
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosTrayService.mm`
- Keeps:
  - service state/lifecycle (`Start/Stop`)
  - localization retrieval
  - helper/factory orchestration

### 4) Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
- New tray helper/factory sources are gated behind `APPLE` to preserve non-Apple compatibility for macOS package scaffolding.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Contract Impact
- No API/schema/user-visible behavior changes.
- Tray settings action and auto-trigger probe semantics remain unchanged.
