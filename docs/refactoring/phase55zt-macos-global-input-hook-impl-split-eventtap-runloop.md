# Phase 55zt: macOS Global Input Hook Impl Split (EventTap/RunLoop)

## Why
- `MacosGlobalInputHook.mm` remained a high-coupling file after helper extraction.
- Lifecycle control, event dispatch callback, and runloop bootstrapping were still mixed in one implementation unit.
- This increased review surface and change risk for permission/runtime hot paths.

## Scope
- Keep external behavior and contracts unchanged.
- Split `MacosGlobalInputHook` implementation into focused translation units:
  - lifecycle/public API
  - event tap callback + permission probe handling
  - runloop startup/teardown
- Keep existing helper modules (`MacosInputEventUtils`, `MacosInputPermissionState`) reused.

## Code Changes

### 1) Promote shared constants to class-level contract
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.h`
- Moved error/polling constants into class static constexpr fields so split files share one source of truth.

### 2) Keep lifecycle/public methods in base impl file
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.mm`
- Retained only:
  - `Start/Stop`
  - `LastError`
  - `ConsumeLatestMove`
  - `SetKeyboardCaptureExclusive`
  - destructor

### 3) Extract event callback and permission probe path
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.EventTap.mm`
- Contains:
  - `EventTapCallback`
  - `PermissionProbeTimerCallback`
  - `OnPermissionProbeTimer`

### 4) Extract runloop startup/teardown path
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.RunLoop.mm`
- Contains:
  - `RunEventTapLoop`

### 5) Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Added new `.mm` units to `mfx_shell_macos`.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Contract Impact
- No API/schema/user-visible behavior changes.
- Internal structure only:
  - lower file-level coupling in macOS global input capture path,
  - clearer ownership for callback vs runloop vs lifecycle changes.
