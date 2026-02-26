# Phase 55zzzzax - macOS Trail Alias Profile/Throttle Parity

## What Changed
- Unified trail alias normalization for profile/throttle selection path:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Config/EffectConfig.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.Shared.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.ClickTrail.cpp`
- `trail` aliases (e.g. `stream`, `neon`, `arc`, `suspension`, `spark`, `default`) now resolve to canonical types before:
  - trail history profile lookup
  - trail throttle policy resolution

## Why
- Fix semantic drift where style rendering used alias-tolerant normalization but profile/throttle path still treated unknown aliases as default line profile.

## Behavior Contract
- No API/schema change.
- Trail effect type normalization is now consistent across:
  - style selection
  - profile derivation
  - throttle derivation

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Four-Capability Mapping
- This change belongs to: `特效` (trail parameter parity hardening).
