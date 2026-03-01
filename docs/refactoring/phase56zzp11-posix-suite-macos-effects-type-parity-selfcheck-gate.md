# Phase 56zzp11: POSIX Suite macOS Effects Type Parity Selfcheck Gate

## What Changed
1. Promoted macOS effects type parity selfcheck to POSIX suite default phase:
   - `run-macos-effects-type-parity-selfcheck.sh --skip-build`
2. Added suite skip switch:
   - `--skip-macos-effects-type-parity-selfcheck`
3. Updated wasm-focused suite wrapper to skip this phase by default (keep wasm lane scoped).

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
- Effects type/alias parity was previously covered by manual script but not guaranteed in default suite.
- Making it a first-class gate reduces regressions where macOS type normalization drifts from shared compute semantics.

## Validation
```bash
./tools/platform/manual/run-macos-effects-type-parity-selfcheck.sh --skip-build --build-dir /tmp/mfx-platform-macos-core-automation-build
./tools/platform/regression/run-posix-effects-regression-suite.sh --platform auto --skip-automation-test
```

Result: both commands passed on macOS host.
