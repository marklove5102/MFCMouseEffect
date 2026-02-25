# Phase 55zi: Core Regression Entry Lock Guard

## Why
- Core regression scripts can target the same runtime binary/process (`mfx_entry_posix_host`).
- Concurrent runs previously risked mutual `pkill` interference, causing false-negative startup failures.

## Scope
- Add a shared lock mechanism for core regression entry scripts.
- Keep existing contract logic unchanged.
- Validate both serial and concurrent invocation behavior.

## Code Changes

### 1) Shared lock primitives in regression common lib
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/common.sh`.
- Added:
  - `mfx_acquire_lock <name> [timeout]`
  - `mfx_release_lock`
  - `mfx_with_lock <name> <timeout> <command...>`
- Implementation notes:
  - lock file via `mkdir`-based lock dir under `${TMPDIR:-/tmp}/mfx-regression-locks`
  - stale lock owner detection (`pid` liveness check) with auto-cleanup
  - timeout guard (default used by callers: 180s)

### 2) Core entry scripts now guarded by the same lock
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-smoke.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-contract-regression.sh`
- Each script now:
  - wraps its workflow with `mfx_with_lock "mfx-entry-posix-host" ...`
  - logs lock usage clearly (`entry host lock: mfx-entry-posix-host`)
  - keeps existing build/run/assert behavior unchanged

## Validation
- `bash -n tools/platform/regression/lib/common.sh`
- `bash -n tools/platform/regression/run-posix-core-smoke.sh`
- `bash -n tools/platform/regression/run-posix-core-automation-contract-regression.sh`
- `bash -n tools/platform/regression/run-posix-core-wasm-contract-regression.sh`
- Concurrent trigger check:
  - run core automation contract script and core wasm contract script in parallel
  - observed lock wait log (`waiting for lock: mfx-entry-posix-host`)
  - both runs completed successfully
- Full suite:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- No endpoint/schema behavior changes.
- This is a regression infrastructure hardening change to remove concurrent-run flakiness.
