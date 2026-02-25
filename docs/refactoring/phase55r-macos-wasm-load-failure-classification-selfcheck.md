# Phase 55r: macOS WASM Load-Failure Classification Selfcheck

## Issue Classification
- Verdict: `Process debt`.
- Problem: Phase 55q introduced machine-readable `last_load_failure_stage/code`, but automated selfcheck only asserted the missing-file (`manifest_io_error`) branch.

## Goal
1. Expand selfcheck to cover all current manifest failure classes.
2. Keep regression assertions deterministic and non-interactive.
3. Prevent silent drift in load-failure classification semantics.

## Implementation
- Extended `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh` invalid-manifest section:
  - `manifest_io_error`: missing manifest file path.
  - `manifest_json_parse_error`: malformed JSON manifest file (`{`).
  - `manifest_invalid`: schema/validation-invalid manifest (`id=""`).
- Each branch now asserts:
  - `ok=false`
  - `last_load_failure_stage=manifest_load`
  - expected `last_load_failure_code`

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Closure
- macOS WASM selfcheck now protects the full load-failure code contract (`io_error/json_parse/invalid`) instead of a single-path assertion.
