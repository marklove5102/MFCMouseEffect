# Agent Current Context (P1, 2026-03-12)

## Purpose
This file is the compact P1 runtime truth for daily execution.  
Deep implementation details are intentionally moved to P2 docs to reduce context waste.

## Scope and Platform Priority
- Primary host: macOS.
- Delivery priority: macOS mainline first, Windows regression-free, Linux compile/contract-level.
- macOS stack rule:
  - New capability modules are Swift-first.
  - Existing `.mm` surface is maintenance/refactor only; avoid expanding `.mm` scope.

## Runtime Lanes
- Stable lane: `scaffold`
- Progressive lane: `core` (`mfx_entry_posix_host`)
- Policy: new cross-platform capability lands in `core` first, while `scaffold` stays stable.

## Active Product Goals
- Goal A: wasm runtime remains bounded-but-expressive (not raw shader ownership), while improving parity and testability.
- Goal B: input indicator and effect plugins coexist safely by lane/surface separation.
- Goal C: automation gesture mapping remains accurate, observable, and low-regression across macOS/Windows.

## Current Capability Snapshot

### Visual Effects / WASM
- `click/trail/scroll/hold/hover` are active in `core`.
- Shared command tail (`blend_mode/sort_key/group_id`) is active.
- Group retained model is active (transform/material/pass remain host-owned bounded surfaces).
- Compatibility boundary remains:
  - wasm can express rich 2D composition.
  - wasm does not own raw GPU pipeline/shader graph.

### Input Indicator
- macOS and Windows indicator labels/streak semantics are aligned (`L xN`, `W+ xN`).
- Indicator wasm dispatch uses dedicated indicator lanes and safer budget floor.
- Indicator plugin routing exposes explicit route snapshot for diagnostics.

### Automation Mapping
- App-scope normalization/parser contracts are stable.
- Gesture mapping supports preset/custom with similarity threshold.
- Custom gesture editor now uses explicit `Draw -> Save` workflow:
  - Drawing is locked until `Draw` is clicked.
  - New strokes persist only on `Save` (saved view is normalized/read-only).
  - Re-edit always requires entering `Draw` mode again.
- Trigger button supports `none` (gesture-only) end-to-end.
- Gesture matching robustness improvements are active (motion-intent arm/high-confidence commit, aspect-ratio normalization, structure-aware scoring).
- Gesture recognizer direction extraction now runs `collect -> simplify -> cap` (instead of early cap), and applies diagonal-bridge jitter cleanup. This specifically improves `W`/multi-turn recognition where early noise previously truncated final intent segments.
- Preset routing now includes generic sub-shape hijack suppression (complex recognized chain vs low-coverage simple candidate), reducing “complex gesture被简单子形状抢命中” across a class of patterns, not only one pair.
- Matching candidate generation is now hybrid:
  - time-window sliding for local intent extraction
  - spatial-window sliding + normalized shape scoring for speed-invariant comparison
- Preset/custom runtime matching now shares one window-aware engine:
  - `best_score / runner_up_score / best_window / candidate_count` emitted by matcher
  - preset + custom both use ambiguity rejection (`best-runner_up >= margin`)
  - custom defaults to stroke-count and stroke-order sensitive matching (no new UI toggle in this phase)
- Route-time candidate selection now has a second-stage specificity policy:
  - when scores are close, larger templates with more turns/segments and better window coverage can outrank simpler contained sub-shapes
  - runner-up ambiguity no longer counts same-template internal windows, only competing candidates
- Core automation HTTP contract now contains gesture-similarity sample suite (`windowed hit / ambiguous reject / custom order / min-length gate`), preventing silent matcher regressions.
- Gesture calibration now has explicit test-friendly overrides with production-safe defaults:
  - `MFX_GESTURE_AMBIGUITY_MARGIN`
  - `MFX_GESTURE_CUSTOM_MIN_EFFECTIVE_STROKE_PX`
  - test API window geometry options (`window_coverage_*`, `window_slide_divisor`)
- Gesture structure extraction now keeps strong turning anchors (`DP + protected-turn retention`) to prevent multi-turn presets from being flattened.
- Current calibration baseline is `6/6` sample pass and no longer shows preset `W` as a failing lane.
- Debug observability now has two layers:
  - `/api/state` snapshot keeps `last_*` summary.
  - New `recent_events` ring buffer (`seq/timestamp/stage/reason/...`) keeps latest route trace history, including window/candidate telemetry.
  - gesture route diagnostics now split `recognized` vs `matched` ids end-to-end, so debug UI can show “当前识别到的形状” and “最终命中的模板” separately instead of overloading one field.
- Buttonless gesture route now has a noisy-motion prefilter (`buttonless_noisy_motion_filtered`) so chaotic/high-speed scribble movement does not enter mapping trigger routing.
- Buttonless route realtime evaluation now keeps re-evaluating the same recognized id while stroke grows (no same-id early return), reducing intermittent “debug recognized gesture pause” and improving late-confidence hit opportunity.
- Pressed-button drag gesture route now also has noisy-motion suppression (`pressed_noisy_motion_filtered`) with stricter thresholds than buttonless, improving stability on fast chaotic drags while preserving normal `V/W` paths.
- Preset matcher now supports dual-direction variants for zig-zag/diagonal families (reverse + mirror + mirror-reverse), reducing direction-habit mismatch in one-way preset shapes.
- Matcher now includes time-resample fallback on top of distance-step sampling, improving fast-stroke segment retention without replacing existing spatial sampling contract.
- Runtime matcher window search now has bounded candidate budgets to avoid long-path candidate explosion:
  - spatial candidates: `spatialWindowMaxCandidates`
  - time candidates: `timeWindowMaxCandidates`
- Gesture window geometry now supports runtime env calibration for both preset/custom dispatch:
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

## Debug and Observability Contract
- Runtime gesture-route diagnostics are gated by debug mode:
  - `./mfx start --debug`
  - or `MFX_RUNTIME_DEBUG=1`
- Default non-debug runs do not emit this lane to avoid overhead.
- WebUI debug rendering is decoupled and mounted in sidebar debug card; runtime state is synced via workspace runtime channel.
- Sidebar debug gesture preview now uses inline SVG stroke/fill attributes (no CSS-only dependency), and the displayed gesture id prefers the latest non-empty `recent_events.gesture_id` to avoid being overwritten by subsequent blank move/skip events.
- Sidebar debug card now prefers explicit fields:
  - `last_recognized_gesture_id` / `recent_events[].recognized_gesture_id`
  - `last_matched_gesture_id` / `recent_events[].matched_gesture_id`
  - `last_matched_gesture_id` is now strict mapping-hit only; no-hit route keeps it empty to avoid stale/false positive "matched gesture" display.
  - old `last_gesture_id` stays as compatibility fallback only.
- Sidebar debug card matched-gesture preview now falls back to latest non-empty `recent_events[].matched_gesture_id`, preventing blank matched panel after intermediate non-hit events overwrite `last_*`.
- Gesture-route diagnostics now include lightweight sampled preview points (`last_preview_points` + `recent_events[].preview_points`) and a path hash. Sidebar debug drawing is now based on runtime sampled trajectory first, with gesture-id template as fallback.
- Debug preview fidelity raised (default cap `180` points) for better visual restoration; still strictly debug-gated and absent in non-debug runtime.
- Sidebar debug preview rendering now uses frontend densify + Chaikin smoothing + larger debug canvas (`108x52`) for less “matchstick” appearance while keeping recognition path unchanged.
- Debug preview fallback path is unified: both id-based and sampled-points rendering now go through resample+dense+smoothing pipeline; end-arrow tangent now uses a stable tail-distance point to avoid occasional “needle/matchstick” artifacts at stroke tail.
- Sidebar debug card no longer falls back to id-template geometry when sampled points are missing; it caches and renders the latest valid sampled trajectory to reduce “火柴棍” fallback artifacts during intermittent diagnostic frames.
- Gesture recognizer diagnostics now carry raw preview trajectory points (`Result.previewPoints`) separate from matching sample points; sidebar debug rendering consumes this raw trajectory to improve stroke fidelity without changing matching behavior.
- Buttonless-gesture realtime loop now stops feeding new recognizer points after a successful trigger (until idle re-arm/reset), preventing left debug preview from growing indefinitely during continuous movement.
- Sidebar recognized/matched gesture preview now keeps full trajectory semantics and applies even downsampling before rendering (instead of tail-only slicing), so start/end direction remains consistent with configured templates while still controlling render cost.
- Buttonless diagnostics now carry short movement preview points even in `awaiting_motion_arm` and `post_trigger_hold` states; sidebar no longer clears preview on `awaiting_motion_arm`, restoring realtime visual feedback during movement.
- Buttonless diagnostics preview now maintains a rolling raw trajectory window (`96` points cap) instead of frame-local 2-point snippets, eliminating “moving but tail gradually disappears” behavior in sidebar realtime preview.
- Sidebar recognized preview selection now prioritizes latest route frame points (`lastPreviewPoints`) over historical recognized-event snapshots, so UI reflects current motion first.
- Custom gesture diagnostics now export flattened multi-stroke preview points for the selected best candidate, so debug preview better reflects composite shapes (e.g., multi-stroke `D`) instead of only the current stroke tail.
- Sidebar debug recognized panel now borrows matched preview as fallback during `custom_trigger/gesture_trigger` when current-frame recognized preview is temporarily empty, reducing intermittent blank frames.
- Diagnostics preview selection is generalized for all gesture types: both preset and custom route updates now prefer best-window sliced preview points (when available) instead of full raw stroke, reducing partial-shape ambiguity and avoiding per-shape special-casing.
- Recognized preview panel fallback is now generic (`recognized || matched`) rather than stage-gated, preventing empty recognized panel for valid matched frames.
- Custom gesture matching in buttonless mode is now generalized for multi-stroke templates: template strokes are flattened into a single ordered polyline for matching against the live single continuous stroke, instead of hard-rejecting `template_stroke_count > 1`.
- Reverted route-level window-sliced diagnostics preview for realtime recognized panel stability; diagnostics preview now keeps full selected trajectory to avoid gradual tail-shortening artifacts during continuous drawing.
- Sidebar debug card now clears stale recognized-gesture preview when route stage is `buttonless_move_skipped` (awaiting arm) or `buttonless_idle_reset`, avoiding “log更新但图形看起来卡住”的误判。
- Debug-only realtime pull is enabled only when:
  - runtime diagnostics payload exists (debug mode),
  - connection is online,
  - focused section is `automation`.
- Polling cadence is adaptive in debug mode:
  - active gesture stages: `~66ms`
  - idle/non-active stages: `~180ms`
  - still only syncs sidebar runtime debug state (no full page rerender).
- WebUI poll bootstrap now also uses idle interval constant (`GESTURE_DEBUG_POLL_MS_IDLE`) to avoid undefined symbol reload failure.

## Server Structure Note
- `MouseFx/Server` has started physical sub-layer split to match SRP rules:
  - `core/`: web settings server lifecycle + request routing/token monitor entry
  - `routes/automation/`: automation + test-automation route handlers
  - `routes/core/`: core API route + request gateway
  - `routes/testing/`: test-only route handlers
  - `routes/wasm/`: wasm catalog/runtime/import/export route handlers and wasm route utils
  - `diagnostics/`: settings diagnostics mapping builders
  - `settings/`: settings schema/state/wasm capabilities mapping
  - `http/`: embedded http server implementation
  - `webui/`: webui assets + webui path resolver
- Compatibility wrapper headers under `MouseFx/Server` have been removed; includes now point to concrete sub-layer paths directly.
- POSIX shell include boundary tightened: `PosixCoreAppShell.h` now forward-declares `mousefx::AppController` / `mousefx::WebSettingsServer`; concrete headers are included in `.cpp`/action units.
- Scaffold POSIX headers also trimmed:
  - `ScaffoldSettingsApi.h` no longer drags `HttpServer.h` / route config include transitively.
  - `ScaffoldSettingsRequestHandler.h` no longer drags `HttpServer.h` transitively.
  - `ScaffoldSettingsRuntime.Internal.h` now uses forward declarations (`HttpServer`, `SettingsRequestHandler`) instead of full includes.

## Regression Gates (High Frequency)
- Scaffold:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- Core effects:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
- Core automation:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- Core wasm:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-wasm-contract-regression.sh --platform auto`
- Full wasm suite:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-wasm-regression-suite.sh --platform auto`
- macOS ObjC++ surface guard:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-macos-objcxx-surface-regression.sh`

## High-Value Manual Commands
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/mfx run`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/mfx start --debug`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-core-websettings-manual.sh --auto-stop-seconds 60`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-automation-injection-selfcheck.sh --skip-build`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-effects-type-parity-selfcheck.sh --skip-build`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-gesture-calibration-sweep.sh --skip-build`

## Contracts That Must Not Drift
- Keep stdin JSON command compatibility.
- Keep wasm ABI backward compatibility inside current major ABI contract unless explicit migration approved.
- Keep host-owned bounded pass/material strategy; do not add raw shader controls without architecture approval.
- Keep docs synchronized in the same change set when behavior/contracts change.

## P2 Detail Routing
Read these only when task keywords match:
- P2 index:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/agent-context/p2-capability-index.md`
- Automation matching and thresholds:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/automation/gesture-matching.md`
- WASM route and ABI:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/custom-effects-wasm-route.md`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/wasm-plugin-abi-v3-design.md`
- Workflow contracts:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/posix-regression-suite-workflow.md`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/posix-core-automation-contract-workflow.md`

## Documentation Governance State
- `current.md` is P1 only (compact execution truth).
- P2 details are indexed and routed by:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/docs/ai-context.sh route --task "<keywords>"`
- Context artifacts:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/.ai/context-index.json`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/.ai/context-map.md`
- Realtime index refresh options:
  - `./tools/docs/ai-context.sh watch`
  - `./tools/docs/install-git-hook.sh`
