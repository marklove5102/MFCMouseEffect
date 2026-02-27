# Phase 55zzzzay - macOS Effects Tempo Profile Parity

## What Changed
- Tuned macOS render-profile tempo mapping for non-GPU effect path:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.ClickTrail.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.ScrollHoldHover.cpp`
- Updated profile scaling:
  - trail duration mapping: `history.durationMs * 0.73` (was `0.55`)
  - hold breathe/rotate tempo: increased to match default profile target rhythm under default config
  - hover breathe/spin tempo: increased to match default profile target rhythm under default config

## Why
- Reduce macOS visible rhythm drift against Windows default effect pacing while keeping the existing architecture and non-GPU implementation unchanged.

## Behavior Contract
- No schema/API changes.
- Only render-profile timing formulas changed.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Four-Capability Mapping
- This change belongs to: `特效` (tempo parameter alignment).
