# Phase 54g: macOS Automation Injection Selfcheck Suite Gate

## Issue Classification
- Verdict: `Regression-gap risk`.
- Problem: `left_click -> Cmd+C` injection path had dedicated selfcheck script, but it was not part of the default POSIX suite; this allowed accidental skip in daily regression runs.

## Goal
1. Promote macOS automation injection selfcheck to a first-class suite phase.
2. Keep deterministic default by running the selfcheck in `--dry-run` mode.
3. Preserve targeted skip control for triage workflows.

## Implementation
- Suite integration:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-regression-suite.sh`
  - add phase:
    - `run-macos-automation-injection-selfcheck.sh --skip-build --dry-run`
    - reuses core build dir (`--core-automation-build-dir` / `--core-build-dir`) when provided.
  - add skip option:
    - `--skip-macos-automation-injection-selfcheck`
  - run condition:
    - macOS host only; non-mac hosts report explicit skip (no failure).

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/platform/manual/run-macos-automation-injection-selfcheck.sh --skip-build --dry-run`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Closure
- macOS automation injection chain now has default suite-level gate coverage, reducing manual-only drift while keeping real-dispatch acceptance separate and explicit.
