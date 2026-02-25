# Phase 55z: Core HTTP WASM Export Filesystem Contract

## Issue Classification
- Verdict: `Process debt`.
- Problem: `export-all` contract checks only validated HTTP fields (`ok`, `count`, `export_path`) but did not verify filesystem-side outcomes.

## Goal
1. Make `export-all` regression assertions verify response-vs-filesystem consistency.
2. Keep assertions deterministic on macOS and compatible with existing suite flow.
3. Preserve Windows behavior by changing only regression scripts.

## Implementation
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`:
  - `_mfx_core_http_assert_wasm_export_all_ok` now additionally asserts:
    - `export_path` parses successfully and exists as a directory.
    - exported top-level directory count under `export_path` matches response `count`.

## Validation
- `bash -n tools/platform/regression/lib/core_http_wasm_helpers.sh`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Closure
- WASM `export-all` regression is now end-to-end contract checked (`HTTP response` + `filesystem materialization`), reducing silent transfer regression risk.
