# Phase 55zzzzbd - macOS Effect Geometry Metric Scaling

## What Changed
- Added shared geometry scaling helper:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosOverlayRenderSupport.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosOverlayRenderSupport.mm`
- New contract:
  - `ScaleOverlayMetric(referenceSize, baseValue, baseReference, minValue, maxValue)`
- Replaced fixed pixel constants with size-proportional metrics in non-GPU effect layers:
  - click:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRendererCore.Layers.mm`
    - base/stroke/text font and star inset/line width now scale with `plan.size`
  - scroll:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Layers.mm`
    - helix/twinkle inset expansion and line width now scale with overlay size
  - trail:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Layers.mm`
    - outer/dot/glow inset and line widths now scale with overlay size
  - hover:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Plan.mm`
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Layers.mm`
    - ring/ring2 inset and stroke widths now scale with overlay size
  - hold:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.Start.mm`
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.Update.mm`
    - ring inset and line width (including progress delta) now scale with hold overlay size

## Why
- Multiple effects used fixed constants tuned for one baseline size.
- On very small/large window sizes, these constants created visible ratio drift (too thick/thin strokes, too tight/loose insets).
- Shared scaling keeps style ratios more stable across size changes and display environments.

## Behavior Contract
- No API/schema changes.
- No platform capability changes.
- Visual tuning policy changed only: geometry metrics are size-proportional with bounded clamps.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
  - PASS (scaffold lane, core lane smoke, core automation contract, macOS injection selfcheck, macOS wasm selfcheck, Linux compile gate, webui semantic tests)

## Four-Capability Mapping
- This change belongs to: `特效` (geometry ratio stability across sizes).
