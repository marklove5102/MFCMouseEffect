# Phase 55zzzzbp - macOS Trail/Hold Color Profile Parity

## Background
- macOS `trail` and `hold` overlays still used hardcoded style colors.
- This created visual drift with Windows when users changed runtime effect config.

## Decision
- Keep existing effect architecture and renderer boundaries.
- Extend render profile contracts so color inputs become explicit profile fields:
  - `TrailRenderProfile` adds per-type color slots (`line/streamer/electric/meteor/tubes/particle`).
  - `HoldRenderProfile` adds base/style color slots (`left/right/middle` + `lightning/hex/hologram/quantum/flux/tech-neon`).
- Resolve these colors from runtime config in profile resolvers; renderer/style modules consume profile values only.

## Implementation
1. Profile contracts
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.ClickTrail.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.ScrollHoldHover.cpp`
- Mapping:
  - trail colors derive from `config.trail.color` + `config.ripple.left/right/middle` blends.
  - hold base/style colors derive from `config.ripple.left/right/middle` and brightness/blend variants.

2. Trail style/render path
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayStyle.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayStyle.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Layers.mm`
- `TrailStrokeColor` / `TrailFillColor` now require `TrailRenderProfile`.

3. Hold style/render path
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayStyle.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayStyle.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.Start.mm`
- `HoldBaseColor` now resolves from `HoldRenderProfile` instead of hardcoded constants.

## Validation
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`

## Impact
- Capability: `特效（trail + hold）`
- User-visible:
  - macOS trail/hold colors now follow runtime profile/config mapping.
  - existing effect type semantics remain unchanged; only color source is normalized to profile-driven.
- Windows/Linux behavior unchanged.
