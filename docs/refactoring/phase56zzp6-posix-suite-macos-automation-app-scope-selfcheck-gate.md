# Phase 56zzp6: POSIX Suite macOS Automation App-Scope Selfcheck Gate

## What Changed
1. Added a first-class POSIX suite phase for macOS automation app-scope alias selfcheck.
2. Extended suite CLI options with `--skip-macos-automation-app-scope-selfcheck`.
3. Wired wasm-focused suite wrapper to skip the new app-scope phase by default, keeping wasm lane scoped.

## Files
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/posix_suite_options.sh`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/posix_suite_phases.sh`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-regression-suite.sh`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-wasm-regression-suite.sh`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/posix-regression-suite-workflow.md`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/agent-context/current.md`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/README.md`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/README.zh-CN.md`

## Why
- `process:code` / `process:code.app` / `process:code.exe` alias consistency is a core automation compatibility contract on macOS.
- It previously existed only as a manual script; promoting it to suite phase makes regressions fail fast.

## Validation
```bash
./tools/platform/regression/run-posix-effects-regression-suite.sh --platform auto --skip-automation-test
```

Expected:
1. macOS host executes:
   - automation injection selfcheck phase
   - automation app-scope selfcheck phase
2. wasm-focused suite still skips app-scope selfcheck by default.
