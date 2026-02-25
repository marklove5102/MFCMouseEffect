# Phase 55za: Core HTTP WASM Export Manifest Integrity Contract

## Issue Classification
- Verdict: `Process debt`.
- Problem: `export-all` contract checks already asserted `count` and export directory existence, but did not validate exported plugin manifest integrity.

## Goal
1. Extend regression assertions to verify exported manifest materialization quality.
2. Keep checks deterministic and platform-safe in existing POSIX suite flow.
3. Preserve runtime behavior (regression-only hardening).

## Implementation
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`:
  - in `_mfx_core_http_assert_wasm_export_all_ok`, added:
    - exported `plugin.json` count under `export_path/*/plugin.json` must equal response `count`,
    - each exported `plugin.json` must be non-empty.

## Validation
- `bash -n tools/platform/regression/lib/core_http_wasm_helpers.sh`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Closure
- WASM `export-all` regression now enforces manifest integrity-level checks in addition to response/dir-count checks, reducing silent packaging-transfer regressions.
