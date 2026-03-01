# Phase56zzp2: macOS Effects Type Normalization Single Source

## Summary
- Category: `Effects` (click/trail/scroll/hover/hold)
- Goal: remove macOS style-layer duplicated type normalization and use Core `Normalize*EffectType(...)` as the single source.
- Scope: macOS style helper files only; no renderer-backend rewrite.

## Why
- Previous macOS style files normalized type aliases locally.
- Core compute already owns canonical normalization used by Windows/contract checks.
- Keeping two normalization paths risks drift (`settings -> active type -> runtime command`) and causes cross-platform parity regressions.

## Changes
1. `MacosTrailPulseOverlayStyle.cpp` now uses `NormalizeTrailEffectType(...)`.
2. `MacosScrollPulseOverlayStyle.cpp` now uses `NormalizeScrollEffectType(...)`.
3. `MacosHoverPulseOverlayStyle.cpp` now uses `NormalizeHoverEffectType(...)`.
4. `MacosClickPulseOverlayStyle.cpp` now uses `NormalizeClickEffectType(...)`.
5. `MacosHoldPulseOverlayStyle.cpp` now uses `NormalizeHoldEffectType(...)`.

## Behavior Impact
- User-visible intent: unchanged.
- Structural effect: all five categories now resolve type aliases from Core normalization first, reducing mac-vs-win drift risk.

## Validation
1. Build:
   - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
2. Effects contract:
   - `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
3. Full POSIX effects suite:
   - `./tools/platform/regression/run-posix-effects-regression-suite.sh --platform auto`

All passed on macOS host.
