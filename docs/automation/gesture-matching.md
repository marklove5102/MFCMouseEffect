# Gesture Matching

Updated: 2026-03-13

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
- unified window-aware candidate scoring (`best/runner-up/window/candidate_count`) for preset + custom
- recognizer direction-chain cleanup before id generation (`collect -> simplify -> cap`)
- route-time winner selection is no longer raw-score-only; it now considers template complexity and matched-window coverage so larger gestures are not easily intercepted by simpler sub-shapes

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

On top of the weighted blend, runtime now applies four extra controls:

- structural mismatch penalty
- time-window sliding search (start/end move by time; finds local intent inside long traces)
- windowed local-match search (single stroke and per-stroke local windows for custom multi-stroke)
- ambiguity rejection (`best - runner_up >= margin`)

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
- turn-angle profile score (per-turn angle magnitude + signed-turn consistency)
- turn-rhythm score (progress interval distribution between turns)

### Structural Mismatch Penalty

The matcher now subtracts a dedicated penalty when two shapes are close in raw path score but clearly belong to different structural families.

Current penalty focuses on:

- segment-count gap
- turn-count gap
- straight-vs-corner-vs-zigzag family mismatch
- diagonal-segment ratio mismatch
- single-turn asymmetry mismatch for `V`-like families
- low segment-code agreement

Practical effect:

- `V`-like input is less likely to leak into `向下后向右 / down_right`
- `W`-like input is less likely to leak into `V`
- flat noisy zig-zag is less likely to cross the preset threshold accidentally

This is intentionally a penalty instead of only another bonus so that shape-DTW cannot fully override obvious structural disagreement.

### Preset Ambiguity Rejection

Preset and custom dispatch both reject a candidate when:

- top-1 preset score is too close to top-2 preset score
- and the winner is not strong enough to justify dispatch certainty

This applies to pressed-button routing as well, not only `trigger_button=none`.

Practical effect:

- if a captured gesture sits between two preset families, runtime now prefers “no trigger” over “wrong trigger”
- a clear winner still dispatches normally

### Generic Sub-Shape Hijack Suppression

To avoid a family-wide class of failures (complex gesture being intercepted by a simpler sub-shape):

- runtime now compares recognizer chain complexity vs candidate template complexity
- if recognizer complexity is clearly higher (`+2` direction units or more) but candidate only explains a partial local window (low coverage), that candidate is rejected before dispatch ranking

This rule is generic and applies to the whole preset family set, not only a single pair like `W/V`.

## Preset Dual-Direction Variants

Preset matching now supports dual-direction variants for zig-zag/diagonal families:

- reverse-direction variant
- horizontal-mirror variant
- mirror+reverse variant

These variants are evaluated under the same matcher and ambiguity policy, so different drawing direction habits (for example right-to-left `W`) can still match the intended preset family without pair-specific patches.

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

Current tuning note:

- A hand-drawn `✓` can still match the `V` preset when directional intent is consistent and threshold allows it; this is intentional to keep practical usability.
- The matcher now prioritizes multi-turn structure/rhythm separation to reduce false positives between `W` and generic zig-zag noise.

Scoring range remains `0..100`.

## Recognizer Direction-ID Guardrail

To avoid `W`/multi-turn gestures being cut by early jitter:

- recognizer now collects more raw direction transitions first
- then applies chain simplification (including diagonal-bridge cardinal jitter cleanup)
- only then enforces `maxDirections` cap (tail-preferred)

This keeps later intended turns visible in `recognized_gesture_id` and improves downstream preset selection stability.

Additional canonicalization rule:

- `diag_down_right_diag_up_right_diag_down_right_diag_up_right` is normalized to preset `w` id (`diag_down_right_diag_up_right_diag_down_right`) for routing/diagnostics consistency.

## Custom Gesture = First-Class Runtime

Custom matching now uses the same core matcher as preset and keeps existing config contract:

- `AutomationKeyBinding.gesturePattern.customStrokes` remains unchanged
- default runtime policy is strict:
  - stroke count sensitive
  - stroke order sensitive
  - per-stroke minimum effective length required (noise-stroke suppression)
- `trigger_button=none` still applies the existing high-confidence guard on top of custom matching

## Calibration Knobs (Prod + Test)

### Production defaults

- pressed-button route (`left/right/middle drag`) window search:
  - spatial coverage: `18%..100%` (step `7%`, slide divisor `5`)
  - max spatial candidates: `132`
  - time window duration: `140..1750ms` (step `110ms`)
  - time anchor step: `60ms`
  - max time candidates: `96`
  - time resample fallback: `22ms` grid, max `108` points
- buttonless route (`trigger_button=none`) window search:
  - spatial coverage: `22%..100%` (step `8%`, slide divisor `5`)
  - max spatial candidates: `88`
  - time window duration: `160..1450ms` (step `120ms`)
  - time anchor step: `70ms`
  - max time candidates: `72`
  - time resample fallback: `26ms` grid, max `88` points
- ambiguity margin: adaptive by best score
  - `>=92 -> 1.5`
  - `>=86 -> 2.5`
  - `>=80 -> 3.5`
  - else `5.0`
- custom min effective stroke length: `18px`

### Test-mode overrides (debug/selfcheck only)

- env override for ambiguity margin:
  - `MFX_GESTURE_AMBIGUITY_MARGIN=<double>` (valid range `0.5..20.0`)
- env override for custom min effective stroke:
  - `MFX_GESTURE_CUSTOM_MIN_EFFECTIVE_STROKE_PX=<double>` (valid range `0..300`)
- env overrides for runtime window geometry and candidate budgets:
  - `MFX_GESTURE_WINDOW_COVERAGE_MIN_PERCENT`
  - `MFX_GESTURE_WINDOW_COVERAGE_MAX_PERCENT`
  - `MFX_GESTURE_WINDOW_COVERAGE_STEP_PERCENT`
  - `MFX_GESTURE_WINDOW_SLIDE_DIVISOR`
  - `MFX_GESTURE_WINDOW_SPATIAL_MAX_CANDIDATES`
  - `MFX_GESTURE_WINDOW_TIME_MIN_MS`
  - `MFX_GESTURE_WINDOW_TIME_MAX_MS`
  - `MFX_GESTURE_WINDOW_TIME_STEP_MS`
  - `MFX_GESTURE_WINDOW_TIME_ANCHOR_STEP_MS`
  - `MFX_GESTURE_WINDOW_TIME_MAX_CANDIDATES`
- env overrides for time-resample fallback:
  - `MFX_GESTURE_TIME_RESAMPLE_STEP_MS`
  - `MFX_GESTURE_TIME_RESAMPLE_MAX_POINTS`
- test API override for window search geometry:
  - `/api/automation/test-gesture-similarity` request `options`:
    - `window_coverage_min_percent`
    - `window_coverage_max_percent`
    - `window_coverage_step_percent`
    - `window_slide_divisor`
    - `time_window_min_ms`
    - `time_window_max_ms`
    - `time_window_step_ms`
    - `time_window_anchor_step_ms`
    - `time_window_max_candidates`

These overrides are opt-in and do not affect default behavior unless explicitly set.

## Buttonless Gesture Contract

`trigger_button=none` is now intentionally more conservative than pressed-button gestures.

### Arming

Buttonless recognition no longer requires a pre-pause before drawing.

It now starts when movement intent is clear:

- first move only caches anchor point
- arm when pointer displacement from anchor reaches minimum distance (`16px`)
- idle reset still applies (`320ms`) to close stale sessions

Practical meaning:

- user can draw immediately without "stop first"
- tiny micro-moves still do not arm a gesture session

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

Realtime behavior note:

- buttonless route no longer short-circuits only because current recognized id equals the previous one.
- as the same stroke keeps growing, runtime keeps re-evaluating windows so debug-side recognized updates stay continuous and dispatch can occur when confidence becomes sufficient later in the same stroke.

### Noisy-Motion Filter (Buttonless)

Before buttonless route dispatch, runtime now drops clearly chaotic strokes and does not enter gesture trigger routing.

Current filter uses combined signals:

- high turn density (many sharp turns per short path span)
- closed scribble ratio (very low start/end distance vs path length)
- high average speed + high turn density together

Filtered routes emit diagnostics reason:

- `buttonless_noisy_motion_filtered`

### Noisy-Motion Filter (Pressed Drag)

Pressed drag route now has an additional conservative noisy-motion rejection before preset/custom routing:

- stage reason: `pressed_noisy_motion_filtered`
- strategy:
  - stricter than buttonless (higher min points / higher turn-density requirement)
  - only blocks chaotic scribble-like drag traces, does not target normal intentional gesture paths

This is a stability guard for accidental fast scribble drag, not a replacement for preset/custom similarity thresholds.

## Runtime Impact

Expected visible behavior changes:

- `无按键/仅手势` is harder to trigger accidentally
- borderline strokes may require a slightly cleaner shape than before
- deliberate gestures remain supported, but random micro-movements should stop injecting shortcuts

## Debug Telemetry Contract

When runtime diagnostics are enabled (`./mfx start --debug` or `MFX_RUNTIME_DEBUG=1`):

- `/api/state.input_automation_gesture_route_status` still provides `last_*` snapshot fields for compatibility.
- A trace ring buffer is now exported as `recent_events` (latest 30 entries).

Each event includes:

- `seq`: monotonic event sequence id in this process
- `timestamp_ms`: monotonic timestamp
- `stage` / `reason`: route stage and decision reason
- `gesture_id`: compatibility field (`matched` if available, otherwise `recognized`)
- `recognized_gesture_id`: live/current recognizer output
- `preview_points`: lightweight sampled trajectory points for realtime debug drawing
- `preview_path_hash`: dedupe key so path-shape changes are observable even when other fields stay the same

Preview fidelity policy:

- preview trajectory collection/rendering is debug-only (`RuntimeDiagnosticsEnabled` gate).
- non-debug runtime does not emit these preview samples.
- default preview cap is higher for shape fidelity (`180` points), and can be tuned by:
  - `MFX_GESTURE_DEBUG_PREVIEW_MAX_POINTS` (`32..512`)
- `matched_gesture_id`: mapping-hit gesture id only (empty when binding not matched)
- `trigger_button`
- `matched` / `injected`
- `used_custom` / `used_preset`
- `sample_point_count`
- `candidate_count`
- `best_window_start` / `best_window_end`
- `runner_up_score`
- `modifiers` (`primary/shift/alt`)

To avoid high-frequency spam on repeated identical states, events are appended only when the diagnostic payload changes versus the last recorded event.

For pressed-button gesture drawing (for example right-button drag):

- runtime now emits `stage=gesture_drag_snapshot` during move sampling
- `reason=collecting` means points are still accumulating and no stable gesture id is available yet
- `reason=gesture_id_ready` means the live sampled path already produced a current gesture id candidate
- after button-up, runtime preserves the final route result in diagnostics for a short hold window (`~900ms`) so the panel keeps showing the end result instead of being immediately overwritten by subsequent move noise

Also, when any pointer button is held down, buttonless diagnostics no longer override the panel with noisy `buttonless_*` skip states.
When buttonless mapping is not enabled, move events no longer emit repetitive `buttonless_disabled` diagnostics.

### Debug UI Rendering (frontend)

In the sidebar debug card, `last_gesture_id` is no longer shown only as raw text.

Current rendering is:

- backend emits canonical gesture ids plus explicit recognized/matched split
- frontend shows two lanes:
  - recognized gesture: current/live recognizer result
  - matched gesture: final selected template
- each lane renders:
  - compact SVG track (centered path + start marker + end tangent arrow)
  - human-readable arrow hint (`↓↘↗`, `→`, etc.)

This keeps backend diagnostics stable while making gesture review readable for runtime testing.

## Regression Checklist

When touching this lane again, verify:

1. random pointer movement with `trigger_button=none` does not inject
2. short accidental strokes stay below guard threshold
3. deliberate preset gestures still inject
4. custom multi-stroke gestures still use the same threshold field
5. diagnostics reasons remain meaningful:
   - `awaiting_first_motion_arm`
   - `awaiting_motion_arm`
   - `motion_arm_ready`
   - `buttonless_candidate_too_short`
   - `buttonless_candidate_below_guard_threshold`
   - `buttonless_candidate_ambiguous`
   - `preset_window_injected`
   - `preset_window_ambiguous`
   - `custom_window_injected`
   - `custom_window_ambiguous`

## Automated Sample Regression (Core HTTP Contract)

Core automation contract regression now includes `/api/automation/test-gesture-similarity` sample checks:

- windowed preset hit (target shape embedded in longer stroke)
- ambiguity rejection (two equal candidates should reject by margin)
- containment priority regression (`W` should beat embedded `V` when the larger pattern explains the stroke better)
- custom multi-stroke order sensitivity (strict order on)
- custom min-effective-length gate (noise stroke rejected)

Entry:

- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_gesture_similarity_checks.sh`
- wired by:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_checks.sh`

## Calibration Baseline (macOS)

- sweep script:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-gesture-calibration-sweep.sh`
- baseline report:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/automation/gesture-calibration-baseline.md`

Latest baseline snapshot shows:

- recommended test baseline:
  - `MFX_GESTURE_AMBIGUITY_MARGIN=4`
  - `MFX_GESTURE_CUSTOM_MIN_EFFECTIVE_STROKE_PX=18`
- sweep dataset pass is now `6/6` (including preset `W`).
- anchor extraction for structure scoring now uses `DP + protected-turn retention`, so multi-turn zig-zag families are less likely to be over-simplified.
