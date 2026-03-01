# Phase 56zzp13: VM Suppression Interval Diagnostics Contract Surfacing

## What Changed
1. Added diagnostics API surface for VM suppression interval:
   - `/api/state.input_capture.effects_suspended_vm_check_interval_ms`
2. Declared schema contract key:
   - `/api/schema.input_capture.diagnostic_keys` includes `effects_suspended_vm_check_interval_ms`.
3. Expanded regression/selfcheck assertions:
   - core state checks assert state+schema key presence.
   - macOS VM suppression selfcheck asserts field value equals configured `--check-interval-ms`.
4. Added compatibility diagnostics method on foreground suppression service interface:
   - `IForegroundSuppressionService::CheckIntervalMsForDiagnostics()`
   - macOS returns effective interval.
   - Windows returns current fixed interval.

## Files
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/System/IForegroundSuppressionService.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/System/NullForegroundSuppressionService.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.VmSuppression.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.InputCaptureDiagnostics.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.CapabilitiesSections.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosVmForegroundSuppressionService.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosVmForegroundSuppressionService.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/windows/System/Win32VmForegroundSuppressionService.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/windows/System/Win32VmForegroundSuppressionService.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_state_checks.sh`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-vm-foreground-suppression-selfcheck.sh`

## Why
- After introducing configurable VM suppression interval, effective runtime value needed explicit diagnostics to make behavior verifiable and avoid hidden env drift.

## Validation
```bash
./tools/platform/manual/run-macos-vm-foreground-suppression-selfcheck.sh --skip-build --build-dir /tmp/mfx-platform-macos-core-automation-build --check-interval-ms 30
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --check-scope effects --build-dir /tmp/mfx-platform-macos-core-automation-build
./tools/platform/regression/run-posix-effects-regression-suite.sh --platform auto --skip-automation-test
```

Result: all commands passed on macOS host.
