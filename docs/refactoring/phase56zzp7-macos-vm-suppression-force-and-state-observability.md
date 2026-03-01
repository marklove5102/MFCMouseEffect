# Phase 56zzp7: macOS VM Suppression Force Override and State Observability

## What Changed
1. Added deterministic VM suppression test override:
   - `MFX_VM_FOREGROUND_SUPPRESSION_FORCE=true|false`
   - Implemented in `MacosVmForegroundSuppressionService`.
2. Expanded suppression refresh trigger:
   - `AppController::OnDispatchActivity` now updates VM suppression state on input-capture health timer ticks as well.
3. Added runtime observability:
   - `/api/state.input_capture.effects_suspended_vm` now exposes VM suppression state.
4. Extended regression assertions:
   - state contract now checks `effects_suspended_vm` presence.
   - input-capture transition contract now checks `effects_suspended_vm` default (`false`) in current flow.

## Files
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosVmForegroundSuppressionService.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosVmForegroundSuppressionService.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.InputCaptureDiagnostics.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_input_contract_steps.sh`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_state_checks.sh`

## Why
- VM suppression path previously lacked deterministic test-control and external state visibility.
- Adding a force flag + state exposure makes VM suppression behavior script-verifiable and reduces manual ambiguity.

## Validation
```bash
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --check-scope effects --build-dir /tmp/mfx-platform-macos-core-automation-build
./tools/platform/regression/run-posix-effects-regression-suite.sh --platform auto --skip-automation-test
```

Result: both commands passed on macOS host.
