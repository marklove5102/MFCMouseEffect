# Phase 56e - Windows EffectFactory Normalize Entry Bridge

## Goal
Keep Windows behavior unchanged while unifying effect-type semantic input with shared compute normalizers, reducing cross-platform alias drift.

## Change
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/EffectFactory.cpp`:
  - Added category-level normalize entry for Windows create path:
    - click: `NormalizeClickEffectType`
    - trail: `NormalizeTrailEffectType`
    - scroll: `NormalizeScrollEffectType`
    - hold: `NormalizeHoldEffectType`
    - hover: `NormalizeHoverEffectType`
  - Registry lookup and fallback creator now both consume normalized type.
  - Fallback creators no longer re-normalize internally (single normalize decision point).

## Why
- M1/M56 mac path already moved to compute-command model.
- Windows keeps current renderer backend, but type semantics should come from the same normalization source to avoid `raw type` vs `normalized type` divergence.

## Validation
- mac build/regression safety check:
  - `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
  - passed

## Risk
- Low:
  - Windows runtime behavior should remain equivalent for existing canonical types.
  - Alias handling becomes more consistent at factory boundary.
