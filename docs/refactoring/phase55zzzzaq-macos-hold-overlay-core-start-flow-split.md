# Phase 55zzzzaq - macOS Hold Overlay Core Start Flow Split

## What Changed
- Extracted `StartHoldPulseOverlayOnMain(...)` from `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.mm` into a dedicated unit:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.Start.mm`
- Kept `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.mm` focused on close/lifecycle teardown (`CloseHoldPulseOverlayOnMain`).
- Updated macOS platform build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Why
- Continue renderer-core responsibility split (single responsibility, lower coupling).
- Separate start-path construction/animation logic from close-path resource teardown to reduce change risk.

## Behavior Contract
- No user-visible behavior change intended.
- Hold overlay start/close runtime semantics remain unchanged; only implementation boundaries changed.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Four-Capability Mapping
- This change belongs to: `特效` (effects rendering pipeline hardening).
