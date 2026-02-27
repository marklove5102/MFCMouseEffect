# Phase 55zzzzba - macOS Effect Overlay Frame Clamp (Multi-screen/Edge Safety)

## What Changed
- Added shared overlay-frame clamping helper in:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosOverlayRenderSupport.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosOverlayRenderSupport.mm`
- New shared contract:
  - `ClampOverlayFrameToScreenBounds(desiredFrame, overlayPt)`
  - Chooses target screen by `overlayPt` and clamps overlay frame origin/size into that screen bounds.
- Wired all non-GPU effect categories to shared clamp path:
  - click: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRendererCore.Plan.mm`
  - scroll: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Plan.mm`
  - trail: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Plan.mm`
  - hover: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Plan.mm`
  - hold(start/update): `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.Start.mm`, `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.Update.mm`

## Why
- Before this slice, each effect built overlay frame directly from center point, without unified edge/multi-screen guard.
- That could produce partial off-screen windows and category-specific inconsistencies near display edges.
- This change establishes one reusable frame-boundary policy across all macOS non-GPU effect overlays.

## Behavior Contract
- No API/schema changes.
- Rendering styles/timing unchanged.
- Only overlay window placement policy changed: keep overlay windows fully inside selected screen bounds.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
  - PASS (scaffold lane, core lane smoke, core automation contract, macOS injection selfcheck, macOS wasm selfcheck, Linux compile gate, webui semantic tests)

## Four-Capability Mapping
- This change belongs to: `特效` (overlay placement stability, multi-screen/edge consistency).
