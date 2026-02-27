# Phase 55zzzzbi - POSIX Regression CLI Value Contract Hardening

## Background
- Several POSIX regression entry scripts accepted value flags via `${2:-}` without strict missing-value detection.
- Misuse like `--platform --skip-http` could be parsed as a value and fail later with low-signal errors.
- `--jobs` validation was not consistently enforced across entry scripts.

## Decision
- Add shared CLI value validation helpers in regression common layer and apply them to all POSIX entry scripts.
- Fail fast with explicit messages at parse time.

## Implementation
1. Shared helpers
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/common.sh`:
  - `mfx_require_option_value <flag> <value>`
  - `mfx_require_positive_integer <value> <context>`

2. Script-level adoption
- Updated all relevant POSIX entry scripts to call `mfx_require_option_value` for value-required flags:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-scaffold-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-smoke.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-contract-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-path-contract-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-linux-compile-gate.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/posix_suite_options.sh`

3. Jobs contract unification
- `--jobs` now uses one validation rule (`positive integer`) in both suite and linux gate entries.

## Validation
- Syntax:
  - `bash -n tools/platform/regression/lib/common.sh`
  - `bash -n tools/platform/regression/lib/posix_suite_options.sh`
  - `bash -n tools/platform/regression/run-posix-*.sh`
- Failure-path checks:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform --skip-scaffold` must fail with `missing value for --platform`
  - `./tools/platform/regression/run-posix-linux-compile-gate.sh --jobs abc` must fail with `invalid --jobs value`
- Regression:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto --enforce-no-objcxx-edits --skip-core-smoke --skip-core-automation --skip-macos-automation-injection-selfcheck --skip-macos-wasm-selfcheck --skip-linux-gate --skip-automation-test`

## Impact
- Capability: `跨平台回归基础设施（支撑特效 / 键鼠指示 / 手势映射 / WASM）`
- No behavior change for valid invocations.
- Better failure determinism for invalid CLI usage.
