# Phase 56zzm - macOS Streamer Continuous Trail Path

## Scope
- Capability: `effects` (trail category, `streamer` subtype).
- Goal:
  - remove visible "matchstick" segmentation for streamer-style trail.
  - keep existing non-streamer pulse styles unchanged.

## Decision
- Route `streamer` to the same continuous line-trail overlay runtime as `line`.
- Keep style differences via config:
  - streamer color uses `renderProfile.streamer.strokeArgb`
  - streamer-specific duration/idle-fade/segment-step tuning.
- Keep pulse renderer path for `electric/meteor/tubes/particle`.

## Code Changes
1. Continuous path routing update:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEffect.cpp`
  - treat `streamer` as `continuousTrail` (`line || streamer`).
  - streamer continuous config:
    - duration scale: `1.9x`
    - duration clamp: `[320, 1800] ms`
    - line width scale: `0.9x` (clamp `[1.5, 18]`)
    - idle fade lower bounds: `start>=180`, `end>=420`
    - interpolation step: `3.0 px`
  - retains teleport guard and planner path for non-continuous types.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`

All commands passed on macOS.

## Result
- Streamer trail now renders as continuous overlay path instead of segmented pulse windows.
- Existing core/scaffold/contracts and ObjC++ gates remain green.
