# Phase 53ae - AppController Input-Capture Runtime Split

## Background
- `AppController.cpp` had grown close to 1k lines again and mixed lifecycle, routing, and input-capture degradation/runtime recovery logic.
- The input-capture logic is cross-cutting (`effects`, `input indicator`, `automation mapping`) and changes frequently during macOS permission hardening.
- Keeping it in the main file increased review risk and made Windows/POSIX build ownership less explicit.

## Decision
- Split input-capture runtime/state logic into a dedicated compilation unit:
  - `AppController.InputCapture.cpp`
- Keep runtime behavior unchanged.
- Wire both CMake (POSIX lane) and Visual Studio project files (Windows lane) in the same change to avoid lane drift.

## Code Changes
1. Extracted input-capture runtime implementation
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.InputCapture.cpp`
- Moved implementations from `AppController.cpp`:
  - `InputCaptureStatus`
  - `EffectsSuspendedByInputCapture`
  - `SetInputCaptureStatusCallback`
  - `NotifyInputCaptureStatusChanged`
  - `ClassifyInputCaptureFailure`
  - `RefreshInputCaptureRuntimeState`
  - `EnterInputCaptureDegradedMode`

2. Shrunk main controller unit
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
- Removed extracted definitions and stale local constant usage.
- File-size reduction after split:
  - `AppController.cpp`: 948 -> 797 lines.

3. Cross-platform build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
  - Added `AppController.InputCapture.cpp` to POSIX core runtime sources.
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MFCMouseEffect.vcxproj`
  - Added `ClCompile` entry for `AppController.InputCapture.cpp`.
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MFCMouseEffect.vcxproj.filters`
  - Added source-filter entry for `AppController.InputCapture.cpp`.

## Behavior Compatibility
- No API/contract behavior changes.
- Permission degradation/recovery semantics remain unchanged.
- Build ownership is now explicit for both POSIX and Windows lanes.

## Functional Ownership
- Category: `特效` + `键鼠指示` + `手势映射` (shared input-capture runtime path).
- Coverage: input-capture degraded/recovery state orchestration used by all three capabilities.

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

2. `./tools/docs/doc-hygiene-check.sh --strict`
- Result: passed.
