# Phase 55zr: WASM Dispatch Readiness Retry Hardening

## Why
- `test-dispatch-click` assertions occasionally failed with missing `"invoke_ok":true` under transient startup timing.
- This created intermittent false-negative failures in manual/selfcheck and suite flows.

## Scope
- Add retry-based readiness assertion helpers for WASM test-dispatch checks.
- Reuse helpers in both regression and macOS manual selfcheck paths.
- Keep endpoint contracts unchanged; only harden assertion timing.

## Code Changes

### 1) Regression helper: dispatch readiness assertion with retry
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`.
- Added:
  - `_mfx_core_http_wasm_test_dispatch_http_code`
  - `_mfx_core_http_assert_wasm_test_dispatch_ok`
- New env tuning:
  - `MFX_CORE_HTTP_WASM_DISPATCH_TIMEOUT_SECONDS` (default: `5`)
  - `MFX_CORE_HTTP_WASM_DISPATCH_RETRY_INTERVAL_SECONDS` (default: `0.2`)

### 2) Core WASM contract checks switched to helper
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_contract_checks.sh`.
- Replaced one-shot dispatch assertions with retry helper call.
- macOS path now requests rendered output readiness in the same retry assertion.

### 3) Manual WASM selfcheck switched to helper
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/lib/wasm_selfcheck_common.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`
- Added helper:
  - `mfx_wasm_selfcheck_test_dispatch_http_code`
  - `mfx_wasm_selfcheck_assert_test_dispatch_ok`
- New env tuning:
  - `MFX_MANUAL_WASM_DISPATCH_TIMEOUT_SECONDS` (default: `5`)
  - `MFX_MANUAL_WASM_DISPATCH_RETRY_INTERVAL_SECONDS` (default: `0.2`)

### 4) Script help text updated
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-contract-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`

## Validation
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_contract_checks.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/lib/wasm_selfcheck_common.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- No API/schema behavior changes.
- Assertion path hardening only: reduces flaky false negatives while preserving existing contract requirements.
