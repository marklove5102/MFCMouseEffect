# Phase 56zzl - macOS Trail Emission Planner (Anti-Matchstick)

## Scope
- Capability: `effects` (trail category).
- Goal:
  - reduce segmented "matchstick" artifacts for non-line trail types.
  - avoid occasional long jump lines (for example corner-to-cursor streaks after large pointer jumps).

## Decision
- Introduce a dedicated emission planner (`MacosTrailPulseEmissionPlanner`) and keep `MacosTrailPulseEffect` focused on orchestration.
- Non-line trail path now uses planner-driven interpolation with bounded segment count.
- Add jump-distance guard (teleport detection): when movement distance is too large, skip rendering for that move and re-anchor the trail cursor.

## Code Changes
1. New planner module:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEmissionPlanner.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEmissionPlanner.cpp`

2. `MacosTrailPulseEffect` refactor:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEffect.h`
  - added planner config state.
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEffect.cpp`
  - initialize planner config in `Initialize()`.
  - add jump guard for large pointer moves.
  - replace inline non-line segmentation loop with planner output (`segmentPoints`).
  - when trail type is `none`, update anchor point then return.

3. Build wiring:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
  - add `MacosTrailPulseEmissionPlanner.cpp` to mac target sources.

## Test-Friendly Runtime Params
The planner exposes explicit test-tuning env vars (defaults + examples):
- `MFX_MACOS_TRAIL_TELEPORT_SKIP_DISTANCE_PX`
  - default: `900`
  - range clamp: `[200, 4000]`
  - test example: `MFX_MACOS_TRAIL_TELEPORT_SKIP_DISTANCE_PX=320` (force quick jump-skip verification)
- `MFX_MACOS_TRAIL_MAX_SEGMENTS`
  - default: `8`
  - range clamp: `[1, 64]`
  - test example: `MFX_MACOS_TRAIL_MAX_SEGMENTS=2` (quickly observe segment-density reduction)

Switch method:
- one-shot shell env before host launch:
  - `MFX_MACOS_TRAIL_TELEPORT_SKIP_DISTANCE_PX=320 MFX_MACOS_TRAIL_MAX_SEGMENTS=2 /tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background`

Regression reset:
- unset both vars or relaunch without them.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- Non-line trail emission no longer depends on dense inline segmentation in effect class.
- Large jump artifacts are guarded by planner threshold.
- Segment density is now bounded and test-tunable.
