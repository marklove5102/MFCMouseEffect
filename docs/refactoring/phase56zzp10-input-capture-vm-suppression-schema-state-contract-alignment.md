# Phase 56zzp10: Input-Capture VM Suppression Schema/State Contract Alignment

## What Changed
1. Added `input_capture` diagnostic key declaration in schema:
   - `/api/schema.input_capture.diagnostic_keys`
   - includes `effects_suspended_vm` and related input-capture fields.
2. Extended state/schema contracts:
   - assert schema contains `input_capture` section.
   - assert `effects_suspended_vm` key exists in schema diagnostic keys.
   - assert `/api/state.input_capture.effects_suspended_vm` is boolean.

## Files
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.CapabilitiesSections.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_state_checks.sh`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/agent-context/current.md`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/README.md`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/README.zh-CN.md`

## Why
- VM suppression runtime field was added to state, but schema did not explicitly declare it.
- Declaring + gating at contract level prevents future schema/state drift.

## Validation
```bash
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --check-scope effects --build-dir /tmp/mfx-platform-macos-core-automation-build
./tools/platform/regression/run-posix-effects-regression-suite.sh --platform auto --skip-automation-test
```

Result: both commands passed on macOS host.
