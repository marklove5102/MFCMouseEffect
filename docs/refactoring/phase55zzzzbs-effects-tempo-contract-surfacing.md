# Phase 55zzzzbs - Effects Tempo Contract Surfacing

## Background
- Type-tempo tuning for macOS trail/scroll/hover is now active in render-plan logic.
- Without explicit contract fields, future refactors can silently drop these per-type tempo differences.

## Decision
- Promote tempo-scale knobs to render-profile contract fields.
- Surface these fields in:
  - `/api/effects/test-render-profiles`
  - `/api/state.effects_profile`
- Add regression assertions for representative tempo keys.

## Implementation
1. Profile contract expansion
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.h`
- Added:
  - `TrailRenderProfile::TypeTempoProfile` and per-type tempo members
  - `ScrollRenderProfile` tempo/size scale members (`default/helix/twinkle`)
  - `HoverRenderProfile` tempo/size scale members (`glow/tubes`)

2. Plan consumers switched to profile tempo fields
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Plan.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Plan.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Plan.mm`

3. API/state surfacing + regression checks
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestEffectsProfileApiRoute.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.EffectsProfileStateBuilder.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_effect_overlay_checks.sh`

## Validation
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`

## Impact
- Capability: `特效（契约可观测性）`
- User-visible behavior unchanged.
- Regression coverage now includes type-tempo parity fields.
