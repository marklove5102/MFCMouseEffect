# Phase 53ad - Effect Suspension Contract Under Input-Capture Degradation

## Background
- macOS permission transitions already had `input_capture.active/reason` regression checks.
- Effect pipeline suspension during degraded mode was implicit (code path existed) but not contract-visible in `/api/state`.
- This made it harder to prove effect-side behavior stayed aligned when permission was revoked/regranted.

## Decision
- Expose an explicit effect suspension signal tied to input-capture degradation.
- Keep user-visible behavior unchanged.
- Extend core automation contract checks to assert effect suspension transitions.

## Code Changes
1. AppController runtime state extension
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.h`
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
- Added atomic state:
  - `effectsSuspendedByInputCapture_`
- Added public accessor:
  - `EffectsSuspendedByInputCapture() const`
- State transitions:
  - set `true` in `EnterInputCaptureDegradedMode(...)`
  - set `false` on capture recovery and successful start
  - reset `false` on `Stop()`

2. State API contract extension
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.Diagnostics.cpp`
- Added field in `input_capture` state:
  - `effects_suspended`

3. Regression gate extension
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`
- Added assertions:
  - startup denied => `effects_suspended=true`
  - startup recovery => `effects_suspended=false`
  - runtime revoke => `effects_suspended=true`
  - runtime regrant => `effects_suspended=false`

## Behavior Compatibility
- No route/path schema removals.
- Existing permission degraded/recovery behavior unchanged.
- Added observability contract for effect-side suspension status.

## Functional Ownership
- Category: `特效`
- Coverage: effect pipeline suspension/resume state under permission-driven input-capture transitions.

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

2. `./tools/docs/doc-hygiene-check.sh --strict`
- Result: passed.
