# phase56v - macOS trail line continuity fix

## What changed
- Fixed macOS `trail=line` path construction to use real movement vector (`previous -> current`) instead of fixed-length centered dashes.
- Expanded trail overlay frame for line-like trail types using movement delta (`deltaX/deltaY`) to prevent path clipping during fast cursor movement.
- Kept stroke/glow metric scaling tied to effect profile size (`command.sizePx`) so large delta frames do not accidentally change visual thickness.

## Why
- Reported behavior showed severe discontinuity: line trail rendered as detached short bars when cursor moved quickly.
- Root cause was fixed short segment path (`len=20`) plus fixed-size frame, which did not represent actual movement span and clipped long deltas.

## Risk
- Larger temporary overlay windows for high-speed movement increase per-frame allocation pressure.
- Electric trail now uses a movement-based kink path; shape is preserved but segment geometry is no longer centered fixed-length.

## Validation
1. Build:
   - `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
2. Manual:
   - Set trail type to `line`.
   - Move cursor at low/high speed.
   - Confirm trail is continuous (no detached short bars).
   - Verify `streamer/electric/meteor` still render and do not clip at fast movement.
