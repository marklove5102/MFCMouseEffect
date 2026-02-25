# Phase 55zq: Core Regression Workflow Helper Consolidation

## Why
- Core regression entry scripts (`smoke`, `automation`, `wasm`) shared the same boilerplate:
  - stale entry-host cleanup
  - core runtime lane build invocation
  - lock-wrapped workflow execution
- Duplicate workflow wiring increases drift risk in future maintenance.

## Scope
- Introduce shared helpers for core entry workflow preparation and lock execution.
- Rewire core regression entry scripts to use helpers.
- Preserve existing behavior and lock semantics.

## Code Changes

### 1) Shared workflow helpers in common library
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/common.sh`.
- Added:
  - `mfx_prepare_core_entry_runtime <context> <repo_root> <build_dir> <platform>`
  - `mfx_run_with_entry_lock <workflow_fn>`

### 2) Core regression entry scripts now call shared helpers
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-smoke.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-contract-regression.sh`
- Replaced repeated stale-cleanup/build/lock boilerplate with helper-based flow.

## Validation
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/common.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-smoke.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-contract-regression.sh`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- No API/schema/runtime behavior changes.
- This is script-infrastructure consolidation that lowers maintenance coupling.
