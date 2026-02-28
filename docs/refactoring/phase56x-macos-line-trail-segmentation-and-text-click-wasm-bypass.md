# phase56x - macOS line trail speed-decouple and text-click wasm bypass

## Classification
- `Bug/regression`
  - `trail=line`: segment length changed with cursor speed (slow short / fast long).
  - `click=text`: no visible response when WASM click route was active.

## Root cause
1. Line trail rendered one command per move with movement-dependent delta; under sparse move cadence, segments became very long.
2. Click router prioritized WASM render route. For `click=text`, built-in `TextEffect` could be skipped when WASM was active.

## Changes
1. `Platform/macos/Effects/MacosTrailPulseEffect.mm`
   - add segmented emission for `line` trail:
     - split one large move into up to 8 sub-segments;
     - nominal step length `~16px`;
     - each segment emits an independent render command.
2. `Platform/macos/Effects/MacosEffectRenderProfile.Shared.h`
   - line throttle changed to `{minIntervalMs=8, minDistancePx=3.0}`.
3. `Platform/macos/Effects/MacosTrailPulseOverlayStyle.mm`
   - line segment clamp tightened to `14..28px` (non-line behavior unchanged).
4. `MouseFx/Core/Control/DispatchRouter.cpp`
   - when active click type normalizes to `text`, force built-in click path and skip wasm click render routing for that event.

## Validation
1. Build:
   - `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
2. Contract:
   - `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
3. Manual:
   - trail type `line`: fast and slow movement should no longer produce extreme long/short segment disparity.
   - click type `text`: text should always appear even with WASM enabled.
