# Gesture Matching

Updated: 2026-03-12

## Why This Exists

Gesture automation had two coupled problems:

1. Matching logic lived inside `InputAutomationEngine.cpp`, mixed with dispatch, app-scope filtering, diagnostics, and injection.
2. `trigger_button=none` (`无按键/仅手势`) sampled live pointer motion too aggressively, so accidental movement could inject a high-risk shortcut such as `Option+Space`.

This document records the new matching boundary and the runtime contract for buttonless gestures.

## Module Boundary

Matching now lives in:

- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Input/GestureSimilarity.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Input/GestureSimilarity.cpp`

The module owns:

- preset template stroke definitions
- custom-template stroke extraction
- captured-stroke metrics
- stroke normalization
- resampling
- stroke similarity scoring
- preset similarity scoring

`InputAutomationEngine.cpp` now keeps only automation-specific orchestration:

- trigger-button routing
- app-scope filtering
- modifier filtering
- dispatch/injection
- diagnostics
- buttonless gesture session state

## Similarity Model

Current stroke score is a weighted blend:

- `30%` DTW-like path-shape score
- `18%` simplified direction-sequence score
- `10%` start/end tangent alignment score
- `42%` structure score

Why:

- path shape improves tolerance to drawing-speed variance
- direction sequence helps separate `v/w/折线`
- tangent alignment reduces mirrored or tail-distorted false positives
- structure score compares simplified anchors, turn layout, segment balance, and single-turn symmetry

### Structure Features

The structure score now uses a simplified polyline extracted from the normalized stroke:

- simplified anchor points (`Douglas-Peucker` + short-segment cleanup)
- segment count and segment direction pattern
- turn-point layout (`x/y/progress`)
- segment-length balance
- single-turn symmetry penalty for `V`-like shapes
- multi-segment intent bonus for longer preset families

### Aspect-Preserving Normalization

Stroke normalization no longer stretches `x` and `y` independently to fill the full `0..100` box.

The runtime now uses:

- uniform scale by the dominant extent
- centering on the smaller axis

Why:

- small vertical jitter on a horizontal line should stay “small jitter”, not be stretched into a fake zig-zag
- diagonal and corner gestures keep their original aspect more faithfully
- preset matching behaves more like the user-drawn paper shape instead of a forced full-box warp

Practical effect:

- `V` is no longer allowed to match a strongly asymmetric `✓`-like stroke just because the path is roughly “down then up”
- `W` keeps benefiting from multi-turn layout instead of depending mostly on raw point distance
- straight-line presets are less sensitive to tiny perpendicular jitter because normalization now preserves aspect ratio

Scoring range remains `0..100`.

## Buttonless Gesture Contract

`trigger_button=none` is now intentionally more conservative than pressed-button gestures.

### Arming

Buttonless recognition no longer starts on every move.

It now requires an idle gap before the stroke is armed:

- arm idle: `180ms`
- idle reset: `320ms`

Practical meaning:

- random pointer motion is ignored while the cursor is still “in motion”
- the user pauses briefly, then starts drawing

### Dispatch Guard

Even after a candidate shape matches, buttonless dispatch is blocked unless all of these pass:

- sample point count is high enough (`>= 5`)
- path length is long enough (`>= max(min_stroke_distance_px * 1.35, 96px)`)
- best score clears the binding threshold plus an extra guard (`+6`)
- best score is sufficiently ahead of the runner-up candidate (`>= 8` margin)

This guard applies to both preset and custom buttonless gestures.

Guard rejection does not permanently lock the current gesture id.

If a live buttonless stroke is still growing, the runtime will re-evaluate the same gesture shape on later move samples until it either:

- passes the guard and injects
- idles out and resets
- changes into a different recognized gesture id

## Runtime Impact

Expected visible behavior changes:

- `无按键/仅手势` is harder to trigger accidentally
- borderline strokes may require a slightly cleaner shape than before
- deliberate gestures remain supported, but random micro-movements should stop injecting shortcuts

## Regression Checklist

When touching this lane again, verify:

1. random pointer movement with `trigger_button=none` does not inject
2. short accidental strokes stay below guard threshold
3. deliberate preset gestures still inject
4. custom multi-stroke gestures still use the same threshold field
5. diagnostics reasons remain meaningful:
   - `awaiting_idle_arm`
   - `buttonless_candidate_too_short`
   - `buttonless_candidate_below_guard_threshold`
   - `buttonless_candidate_ambiguous`
   - `preset_similarity_binding_injected`
   - `custom_binding_injected`
