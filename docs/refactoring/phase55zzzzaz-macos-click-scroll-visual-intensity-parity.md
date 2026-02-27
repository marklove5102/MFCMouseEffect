# Phase 55zzzzaz - macOS Click/Scroll Visual Intensity Parity

## What Changed
- Tuned click/scroll visual intensity profile mapping (non-GPU path):
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.ClickTrail.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.ScrollHoldHover.cpp`
- Click profile updates:
  - larger default window mapping (`normalSizePx`, `textSizePx`)
  - slightly longer normal/text duration mapping
- Scroll profile updates:
  - increased base duration and per-strength duration step
  - increased horizontal/vertical window size mapping

## Why
- Continue macOS-visible parity toward Windows baseline for click/scroll readability and presence, while preserving architecture and existing contracts.

## Behavior Contract
- No API/schema changes.
- Only profile formulas changed.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Four-Capability Mapping
- This change belongs to: `特效` (click/scroll intensity alignment).
