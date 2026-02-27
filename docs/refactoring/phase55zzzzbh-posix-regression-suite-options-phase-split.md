# Phase 55zzzzbh - POSIX Regression Suite Options/Phase Split

## Background
- `run-posix-regression-suite.sh` had accumulated both option parsing and full phase execution flow.
- The script mixed concerns:
  - CLI parsing/validation
  - default value ownership
  - per-phase orchestration
- This increased change blast radius and made future gate additions harder to review.

## Decision
- Split suite responsibilities into dedicated helper modules while keeping the same CLI contract:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/posix_suite_options.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/posix_suite_phases.sh`
- Keep `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-regression-suite.sh` as a thin orchestrator entry.

## Implementation
1. Options module
- centralizes defaults/init (`mfx_posix_suite_init_defaults`)
- centralizes usage output (`mfx_posix_suite_print_usage`)
- centralizes arg parsing (`mfx_posix_suite_parse_args`)
- adds explicit value checks for value-required flags (`--platform`, `--jobs`, build-dir flags)
- adds strict `--jobs` validation (`positive integer`)
- normalizes env/flag toggle semantics for `MFX_ENFORCE_NO_OBJCXX_EDITS` (`0/1` and `true/false` family)

2. Phases module
- extracts each stage into one function:
  - policy gate
  - scaffold
  - core smoke
  - core automation contract
  - macOS automation injection selfcheck
  - macOS wasm selfcheck
  - linux compile gate
  - webui semantic tests
- consolidates repeated core build-dir fallback logic (`core-automation-build-dir -> core-build-dir`)

3. Entry script
- now only performs:
  - shared `common.sh` load
  - options init/parse/export
  - ordered phase invocation

## Validation
- Syntax:
  - `bash -n tools/platform/regression/run-posix-regression-suite.sh`
  - `bash -n tools/platform/regression/lib/posix_suite_options.sh`
  - `bash -n tools/platform/regression/lib/posix_suite_phases.sh`
- Regression:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto --enforce-no-objcxx-edits --skip-core-smoke --skip-core-automation --skip-macos-automation-injection-selfcheck --skip-macos-wasm-selfcheck --skip-linux-gate --skip-automation-test`
- Failure-path contract:
  - `./tools/platform/regression/run-posix-regression-suite.sh --jobs abc` must fail fast with a clear message.

## Impact
- Capability: `跨平台回归基础设施（支撑特效 / 键鼠指示 / 手势映射 / WASM 四类能力）`
- No behavior change in phase execution order or default enabled phases.
- No Windows runtime behavior changes.
