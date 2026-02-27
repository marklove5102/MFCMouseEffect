# Phase 56d - macOS Effect Compute Profile Adapter Consolidation

## Goal
Remove repeated `mac profile -> shared compute profile` mapping logic from multiple effect/renderer files, and keep a single mapping source.

## Change
- Added unified adapter:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectComputeProfileAdapter.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectComputeProfileAdapter.cpp`
- Adapter covers all 5 categories:
  - click / trail / scroll / hover / hold
  - plus trail throttle mapping
- Replaced duplicated local builders in:
  - `MacosClickPulseEffect.mm`
  - `MacosTrailPulseEffect.mm`
  - `MacosScrollPulseEffect.mm`
  - `MacosHoverPulseEffect.mm`
  - `MacosHoldPulseEffect.mm`
  - `MacosClickPulseOverlayRenderer.mm`
  - `MacosTrailPulseOverlayRenderer.mm`
  - `MacosScrollPulseOverlayRenderer.mm`
  - `MacosHoverPulseOverlayRenderer.mm`
  - `MacosHoldPulseOverlayRenderer.mm`
  - `SettingsStateMapper.EffectsProfileStateBuilder.Macos.cpp`

## Why
- Settings/command consistency risk came from many duplicated conversion blocks.
- Consolidation ensures one edit point for parameter/alias evolution and reduces mac-only drift.

## Validation
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
  - build passed
  - effects contract checks passed

## Risk
- Low: refactor-only with no model/protocol shape changes.
- Mitigation: existing effects contract regression keeps command/profile route checks active.
