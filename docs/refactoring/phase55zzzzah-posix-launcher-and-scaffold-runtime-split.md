# Phase 55zzzzah - POSIX Launcher and Scaffold Runtime Split

## Why
- `PosixSettingsLauncher.cpp` mixed input validation, capture-file test mode, and `posix_spawn` execution.
- `ScaffoldSettingsRuntime.cpp` mixed runtime wiring and embedded server start orchestration.
- These mixed responsibilities increase maintenance risk on scaffold/core shell lanes.

## What Changed
- Split POSIX settings launcher by concern:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/PosixSettingsLauncher.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/PosixSettingsLauncher.Capture.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/PosixSettingsLauncher.Spawn.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/PosixSettingsLauncher.cpp` now keeps orchestration only.
- Split scaffold runtime embedded-server start orchestration:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/ScaffoldSettingsRuntime.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/ScaffoldSettingsRuntime.Start.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/ScaffoldSettingsRuntime.cpp` now keeps runtime-state holder/wrapper responsibilities.
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/CMakeLists.txt`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`

## Capability Mapping
- This change belongs to: shell/settings infrastructure (shared support layer for `effects`, `input indicator`, `automation mapping`, `WASM`).
- No direct user-visible behavior changes in the four capability planes.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
  - `./tools/docs/doc-hygiene-check.sh --strict`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- Responsibility split only; URL launch and scaffold settings runtime contracts unchanged.
