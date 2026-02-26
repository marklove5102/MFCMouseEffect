# Phase 55zzzy - AppController VM suppression split

## Summary
- Capability: runtime `effects` / `automation` suppression control path.
- This slice isolates VM suppression state transitions from general effect lifecycle logic in `AppController`.

## Changes
1. Added dedicated VM suppression unit
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.VmSuppression.cpp`
- Owns:
  - `UpdateVmSuppressionState`
  - `ApplyVmSuppression`
  - `SuspendEffectsForVm`
  - `ResumeEffectsAfterVm`

2. Slimmed effects unit
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.Effects.cpp`
- Removed VM suppression method implementations from this file.

3. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added new source unit for runtime target.

## Why
- Previous `AppController.Effects.cpp` mixed effect selection/config logic with suppression state-machine behavior.
- Split reduces coupling and localizes suppression evolution.

## Validation
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/docs/doc-hygiene-check.sh --strict
```

## Compatibility
- No suppression behavior changes intended.
- Existing VM suppression contracts preserved.
