# Phase 53i: macOS Automation Injection Selfcheck and Match-Inject Contract

## Issue Classification
- Verdict: `Manual-acceptance friction`.
- Problem: real key injection acceptance (`left_click -> Cmd+C`) still relied on ad-hoc manual choreography and had no direct scripted contract for `history -> match -> inject`.

## Goal
1. Add a test-gated API that verifies matcher and injector integration in one request.
2. Provide a one-command macOS manual selfcheck script for real OS key dispatch.
3. Keep existing regression deterministic by preserving dry-run support.

## Implementation
1. Matcher + injector test endpoint
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- Route: `POST /api/automation/test-match-and-inject`
- Gate:
  - `MFX_ENABLE_AUTOMATION_SCOPE_TEST_API=1`
  - `MFX_ENABLE_AUTOMATION_INJECTION_TEST_API=1`
- Behavior:
  - parse `history` + `mappings`,
  - reuse shared matcher (`BindingMatchUtils`) to select best binding,
  - execute `InjectShortcutForTest` on selected keys,
  - return `matched`, `injected`, selected binding metadata.

2. Contract regression extension
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`
- Added assertion for endpoint contract:
  - `matched=true`
  - `injected=true`
  - `selected_keys="Cmd+C"`

3. macOS one-command injection selfcheck
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-automation-injection-selfcheck.sh`
- Default mode (`--dry-run` not set):
  - prepare TextEdit selection (`Cmd+A`) with sentinel text,
  - call `/api/automation/test-match-and-inject`,
  - verify clipboard equals sentinel (real OS dispatch evidence).
- Test-friendly mode:
  - `--dry-run` enables `MFX_TEST_KEYBOARD_INJECTOR_DRY_RUN=1`,
  - skips TextEdit/clipboard check and only validates match+inject contract path.

## Test-Friendly Parameters
- Default (production-like manual selfcheck):
  - `dry_run=0` (real injection)
  - `auto_stop_seconds=120` (only with `--keep-running`)
- Test mode:
  - `--dry-run` (deterministic no-system-input mode)
  - `--skip-build` (reuse existing build for fast rerun)
- Switch method:
  - real dispatch: omit `--dry-run`
  - deterministic contract: add `--dry-run`

## Validation
- `bash -n tools/platform/manual/run-macos-automation-injection-selfcheck.sh`
- `tools/platform/manual/run-macos-automation-injection-selfcheck.sh --help`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Closure
- `history -> binding selection -> shortcut inject` is now script-closed in core contracts.
- Real OS-level injection acceptance has a stable one-command path instead of ad-hoc manual steps.
