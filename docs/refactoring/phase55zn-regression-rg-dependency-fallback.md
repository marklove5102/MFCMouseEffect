# Phase 55zn: Regression `rg` Dependency Fallback

## Why
- Some regression/manual runs failed early on machines without `rg` even though equivalent `grep` checks were feasible.
- Existing scripts mixed `rg` hard-requirement and `grep` fallback behavior, increasing environment-friction drift.

## Scope
- Add shared file-match helpers in regression common library with `rg` priority and `grep` fallback.
- Replace direct `rg` calls in core HTTP contract modules with shared helpers.
- Remove `mfx_require_cmd rg` hard dependency from regression entry scripts that no longer need it.

## Code Changes

### 1) Shared match helpers in common lib
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/common.sh`.
- Added:
  - `mfx_file_contains_fixed <file> <pattern>`
  - `mfx_file_contains_regex <file> <pattern>`
- Updated `mfx_assert_file_contains` to reuse `mfx_file_contains_fixed`.

### 2) Core contract modules switched to shared helpers
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_input_contract_checks.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_checks.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_contract_checks.sh`
- Replaced direct `rg -q` checks with `mfx_file_contains_fixed/regex`.

### 3) Removed `rg` hard requirement in entry scripts
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-scaffold-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-contract-regression.sh`
- Removed `mfx_require_cmd rg`.

## Validation
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/common.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_input_contract_checks.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_checks.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_contract_checks.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-scaffold-regression.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-contract-regression.sh`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- No API/schema behavior changes.
- Regression scripts are now less tool-fragile:
  - `rg` remains preferred when present,
  - `grep` fallback avoids hard failure when `rg` is absent.
