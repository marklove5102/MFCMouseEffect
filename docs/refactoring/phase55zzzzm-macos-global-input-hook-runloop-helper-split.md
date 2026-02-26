# Phase 55zzzzm - macOS Global Input Hook RunLoop Helper Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.RunLoop.mm` mixed:
  - init result notify
  - permission simulation loop
  - event-tap mask definition
  - runloop lifecycle orchestration
- This inflated one file with multiple responsibilities.

## What Changed
- Added RunLoop helper module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.RunLoopHelpers.mm`
- Extended class private contract:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.h`
  - added `NotifyInitResult`, `RunPermissionSimulationLoop`, `ComputeEventTapMask`.
- Simplified runloop main file:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.RunLoop.mm`
  - now focuses on tap/runloop lifecycle flow.
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `键鼠指示` + `手势映射/自动化触发` input-hook runtime path.
- Not part of: WASM render path or effect visual style path.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- Behavior contracts unchanged; responsibility split only.
