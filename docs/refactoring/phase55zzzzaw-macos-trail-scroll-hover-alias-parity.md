# Phase 55zzzzaw - macOS Trail/Scroll/Hover Alias Parity

## What Changed
- Updated macOS effect-type normalization:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayStyle.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayStyle.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayStyle.mm`
- Added alias-tolerant matching for cross-platform/legacy naming:
  - trail: `meteor`, `stream/streamer/neon`, `electric/arc`, `tube/suspension`, `particle/spark`, `line/default`
  - scroll: `helix`, `twinkle/stardust`, `arrow/direction/indicator`
  - hover: `tubes/suspension/helix` -> tubes, `glow/breath` -> glow

## Why
- Keep mac effect selection semantics aligned when Windows-origin or historical names appear in config/runtime payloads.

## Behavior Contract
- No API/schema change.
- Only type-normalization behavior widened for compatibility aliases.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Four-Capability Mapping
- This change belongs to: `特效` (trail/scroll/hover semantic parity).
