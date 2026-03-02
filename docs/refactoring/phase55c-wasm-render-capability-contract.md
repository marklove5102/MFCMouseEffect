# Phase55c: WASM Render Capability Contract (Schema/State/Regression)

## Goal
- Continue Phase 55 with explicit capability contracts so UI and tests can distinguish:
  - WASM invoke supported
  - WASM render supported
- Avoid silent ambiguity on macOS where runtime invoke is available but render path is still degraded.

## Status Note
- This phase introduced the contract fields and initial macOS degraded semantics.
- As of Phase 55f, macOS semantics are updated to `invoke=true, render=true` via platform renderer strategy.

## Code Changes
1. Schema capability contract
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.cpp`
  - Added `capabilities.wasm`:
    - `invoke`
    - `render`
  - Extended `wasm.diagnostic_keys` with:
    - `invoke_supported`
    - `render_supported`

2. Runtime state contract
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.cpp`
  - Added `wasm.invoke_supported` and `wasm.render_supported` to `/api/state`.
  - Initial semantics in 55c:
    - Windows: invoke=true, render=true
    - macOS core lane: invoke=true, render=false
  - Current semantics (after 55f):
    - Windows: invoke=true, render=true
    - macOS core lane: invoke=true, render=true

3. Regression guard
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`
  - Added checks for state fields:
    - `invoke_supported`
    - `render_supported`
  - Added schema check for `capabilities.wasm`.
  - Added macOS assertion in 55c:
    - `render_supported=false`
  - Updated in 55f:
    - `render_supported=true`

## Validation
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` passed.
- Existing scaffold/linux/webui regression gates remain green in this branch after this change.

## Impact
- Web/automation tools now have a stable contract to detect "invoke-ready but render-degraded" runtime.
- Reduces confusion when runtime backend is active but visual output is intentionally not executed on POSIX yet.
