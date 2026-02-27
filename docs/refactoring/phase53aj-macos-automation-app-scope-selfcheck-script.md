# Phase 53aj - macOS Automation AppScope Selfcheck Script

## Background
- `process:code.exe` / `process:code` / `process:code.app` alias matching is already covered by regression APIs.
- Manual verification still required repeated host startup, token extraction, and curl payload assembly.
- This increased manual test friction and made app-scope acceptance slower.

## Decision
- Add a one-command macOS manual selfcheck script for app-scope alias consistency:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-automation-app-scope-selfcheck.sh`
- Reuse existing regression helper contracts instead of duplicating request/assert logic.

## Implementation
1. New script startup flow:
- builds `mfx_entry_posix_host` in core lane (unless `--skip-build`)
- acquires shared `mfx-entry-posix-host` lock
- starts tray-mode host with `MFX_ENABLE_AUTOMATION_SCOPE_TEST_API=1`
- auto-resolves `settings_url` and token via probe file

2. Script assertions:
- `code` vs `process:code.exe` => match `true`
- `code.app` vs `process:code` => match `true`
- `code.exe` vs `process:code.app` => match `true`
- `safari` vs `process:code` => match `false`

3. Runtime ergonomics:
- supports `--keep-running` + `--auto-stop-seconds`
- prints deterministic stop command (`kill -TERM <pid>`)

## Validation
- Shell syntax:
  - `bash -n tools/platform/manual/run-macos-automation-app-scope-selfcheck.sh`
- Manual selfcheck:
  - `./tools/platform/manual/run-macos-automation-app-scope-selfcheck.sh --skip-build`
- Regression safety:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto --skip-core-smoke --skip-core-automation --skip-macos-automation-injection-selfcheck --skip-macos-wasm-selfcheck --skip-linux-gate --skip-automation-test`

## Impact
- Capability: `手势映射/自动化（AppScope 命中一致性）`
- No runtime behavior change in production path.
- No Windows behavior changes.
