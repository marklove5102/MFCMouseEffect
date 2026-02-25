# Phase 55u: Core HTTP WASM Load-Failure Contract Assertions

## Issue Classification
- Verdict: `Process debt`.
- Problem: Phase 55q/55s already exposed and selfchecked WASM load-failure stage/code semantics, but `core_http` contract regression did not yet assert these fields end-to-end.

## Goal
1. Extend core HTTP contract regression to assert WASM load-failure diagnostics.
2. Cover success-clear, failure-stage/code, and reload-clear semantics in one regression flow.
3. Reduce duplicate load-manifest request/assert logic via helperized shell functions.

## Implementation
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`:
  - Added helper functions:
    - `_mfx_core_http_wasm_load_manifest_http_code`
    - `_mfx_core_http_assert_wasm_load_manifest_ok`
    - `_mfx_core_http_assert_wasm_load_manifest_failure`
  - Extended `/api/state` diagnostics presence assertions:
    - `last_load_failure_stage`
    - `last_load_failure_code`
  - Extended WASM contract flow:
    - valid load: assert `ok=true` + failure fields cleared
    - invalid manifest path: assert `ok=false` + `manifest_load/manifest_io_error`
    - reload valid manifest: assert failure fields cleared again

## Validation
- `bash -n tools/platform/regression/lib/core_http.sh`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Closure
- Core HTTP regression now enforces WASM load-failure diagnostics contract in addition to manual selfcheck coverage, reducing drift risk between route diagnostics and CI-style contract gates.
