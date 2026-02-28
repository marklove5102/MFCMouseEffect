# Phase56z: macOS line-trail continuity + text-click fallback visibility

## Problem
- macOS `trail=line` rendered as spaced "matchsticks" on fast moves due to sparse emissions.
- macOS `click=text` produced inconsistent or invisible floating text; `NSTextField` introduced unintended background/border in some cases.

## Root Cause
- `MacosTrailPulseEffect` emitted a single overlay per movement sample; large deltas created long, disconnected segments.
- `MacosTextEffectFallback` used `NSTextField` in a transparent panel, which is less reliable for layered overlays and could surface background artifacts.

## Fix
- Line-trail interpolation: emit multiple sub-segments for large deltas in `MacosTrailPulseEffect` (line-only), using size-scaled step length and forced emission when distance is large.
- Text fallback rendering: switch to `CATextLayer` to render floating text with explicit font/color updates per frame, removing `NSTextField` background/border behavior.
- Line-trail persistent overlay: replace per-pulse overlay for `line` with a screen-sized path overlay that accumulates recent points and renders a continuous stroke (Windows-style), then fades out by duration/idle-fade.
- Line-trail lifecycle now keeps the overlay alive briefly even when only one point is present, and uses raw Quartz points + Cocoa conversion inside the overlay to avoid coordinate mismatch.

## Files
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEffect.mm`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosLineTrailOverlay.mm`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosLineTrailOverlay.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTextEffectFallback.mm`

## Validation
- Build: `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
- Manual:
1. Set click effect to `text` and verify floating text shows near click.
2. Set trail effect to `line` and verify continuous line at fast cursor speeds.
