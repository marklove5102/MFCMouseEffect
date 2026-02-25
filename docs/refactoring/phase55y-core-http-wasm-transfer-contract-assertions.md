# Phase 55y: Core HTTP WASM Transfer Contract Assertions

## Issue Classification
- Verdict: `Process debt`.
- Problem: core WASM HTTP regression covered `catalog/import/load/enable/dispatch`, but transfer API coverage was still partial:
  - `import-selected` failure semantics were not asserted.
  - `export-all` success contract was not asserted.

## Goal
1. Extend script-level WASM contract coverage to include transfer APIs end-to-end.
2. Keep helper boundaries clear (`core_http.sh` orchestration, `core_http_wasm_helpers.sh` WASM request/assert details).
3. Preserve existing platform behavior and keep regression suite green.

## Implementation
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`:
  - added `_mfx_core_http_wasm_import_selected_http_code`
  - added `_mfx_core_http_wasm_export_all_http_code`
  - added `_mfx_core_http_assert_wasm_import_selected_ok`
  - added `_mfx_core_http_assert_wasm_import_selected_failure`
  - added `_mfx_core_http_assert_wasm_export_all_ok`
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`:
  - switched `import-selected` assertions to helperized path.
  - added `export-all` contract assertion (`count >= 1` after import-selected success).
  - added invalid `import-selected` path assertion (`manifest_path does not exist`).

## Validation
- `bash -n tools/platform/regression/lib/core_http_wasm_helpers.sh`
- `bash -n tools/platform/regression/lib/core_http.sh`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Closure
- WASM transfer API contract coverage is now closed in core HTTP regression:
  - `import-selected` success + failure semantics are asserted.
  - `export-all` success semantics are asserted with minimum exported-count guard.
