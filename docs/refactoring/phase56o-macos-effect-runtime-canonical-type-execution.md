# Phase 56o - macOS Effect Runtime Canonical Type Execution

## Why
- macOS effect runtime still accepted raw type strings at multiple runtime points.
- Although compute layer already normalizes, constructors and registry entry still had fallback/default handling, increasing branch drift risk.

## What Changed
1. Effect runtime canonicalization at constructor boundary:
- `MacosClickPulseEffect` now stores canonical click type via `NormalizeClickEffectType`.
- `MacosTrailPulseEffect` now stores canonical trail type via `NormalizeTrailEffectType`.
- `MacosScrollPulseEffect` now stores canonical scroll type via `NormalizeScrollEffectType`.
- `MacosHoverPulseEffect` now stores canonical hover type via `NormalizeHoverEffectType`.
- `MacosHoldPulseEffect` now stores canonical hold type via `NormalizeHoldEffectType`.

2. Registry simplification:
- `MacosEffectCreatorRegistry.Table.cpp` hold creator no longer pre-normalizes via route catalog; constructor canonicalization is the single runtime entry.

## Result
- macOS runtime execution path now consistently consumes canonical type ids before event-time rendering.
- This reduces per-layer fallback branching and keeps renderer behavior aligned with shared compute contracts.

## Validation
```bash
./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto
```
