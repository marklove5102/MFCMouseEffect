# Phase 55zzzzbr - macOS Effects Type Tempo Variant Parity

## Background
- Even after color-profile parity, several macOS effects still shared near-identical animation tempo across types.
- Windows effect types have clearer pacing separation (for example: meteor vs electric, helix vs twinkle, glow vs tubes).

## Decision
- Keep current architecture and route contracts unchanged.
- Add type-aware tempo/size scaling in macOS render-plan layer (runtime-only behavior tuning):
  - `trail`: per-type duration and size scale.
  - `scroll`: per-type duration and size scale.
  - `hover`: per-type size + breathe + tubes spin scale.

## Implementation
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Plan.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Plan.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Plan.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Layers.mm`

## Validation
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`

## Impact
- Capability: `特效（trail + scroll + hover）`
- User-visible:
  - effect types now have more distinct motion pacing on macOS.
  - no API/schema contract changes.
- Windows/Linux behavior unchanged.
