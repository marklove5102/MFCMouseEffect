# Phase 55zzzzbn - macOS Click Color Config Parity

## Background
- macOS click overlay colors were hardcoded in renderer style helpers.
- Windows click effect uses config-driven per-button colors from `EffectConfig.ripple`.
- This produced platform drift when users customized click colors.

## Decision
- Keep macOS click effect architecture unchanged (`MacosClickPulseEffect` + render profile path).
- Extend `ClickRenderProfile` to carry per-button color fields sourced from `EffectConfig.ripple`.
- Make click style helpers consume profile colors instead of hardcoded constants.

## Implementation
1. Profile contract expansion
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.ClickTrail.cpp`
- Added `ClickButtonColorProfile` in `ClickRenderProfile`:
  - `leftButton`
  - `rightButton`
  - `middleButton`
- `ResolveClickRenderProfile(...)` now maps:
  - `config.ripple.leftClick`
  - `config.ripple.rightClick`
  - `config.ripple.middleClick`

2. Style rendering path update
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayStyle.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayStyle.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRendererCore.Layers.mm`
- `ClickPulseStrokeColor(...)` / `ClickPulseFillColor(...)` now take profile input and convert ARGB -> `NSColor`.
- Ring/star/text color application now uses profile-derived per-button colors.

## Validation
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`

## Impact
- Capability: `特效（点击）`
- User-visible:
  - macOS click effect now follows runtime config color changes like Windows.
  - Existing defaults remain unchanged when config is default.
- Windows/Linux behavior unchanged.
