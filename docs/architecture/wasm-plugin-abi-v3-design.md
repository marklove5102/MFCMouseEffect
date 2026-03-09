# WASM Plugin ABI v3 Design

## Status
- Status: `Proposed (phase1 partially landed in v2-compatible form)`
- Date: `2026-03-08`
- Goal: move the current WASM route from a controlled burst-command system toward a more capable cross-platform custom-effects runtime

## Background
The current v2 route already has a stable runtime, frame-driven updates, and a usable primitive set:
- `spawn_text`
- `spawn_image`
- `spawn_image_affine`
- `spawn_pulse`
- `spawn_polyline`
- `spawn_glow_batch`
- `spawn_sprite_batch`

This is enough for text, images, pulses, polylines, glow particles, and sprite bursts, but it still falls short of “near-native C++ freedom” in several key areas:
- only a minimal cross-platform retained glow-emitter baseline exists; a more generic retained effect / emitter lifecycle contract is still missing
- no generic path/vector primitive for curves and fills
- no unified layer / blend / clip / sort contract
- no textured quad / atlas / ribbon style primitive
- no host-owned post-process pass surface

## Non-Goals
v3 does not do the following:
- let plugins control windows, swap chains, or native graphics contexts directly
- let plugins inject arbitrary GPU shaders or pipelines
- move safety budgets, fallback, or degradation policy into plugins
- claim full parity with unrestricted native C++ rendering

The core boundary remains:
- `WASM computes`
- `the host renders, owns resources, and enforces safety`

## Design Principles
1. Prefer reusable public primitives
- Do not keep adding effect-style-specific commands when a reusable primitive can cover the same space.

2. Retained effects before more instant bursts
- v2 is still burst-heavy. v3 should first add long-lived instances and emitters.

3. Public capability means Windows + macOS together
- A single-platform implementation may exist experimentally, but it is not a formal v3 public surface until both platforms support it.

4. Keep safety centralized in the host
- Budgets, throttling, fallback, diagnostics, and resource resolution remain host-owned.

5. Expand in layers, not one giant leap
- First retained + path + layer/blend, then mesh-like primitives and post-process.

## v3 Goal Layers

### P1 Required
- retained particle / sprite emitter
- path/vector primitive
- shared layer / blend / clip / sort contract

### P2 Recommended
- textured quad batch / atlas / uv animation
- ribbon / trail strip primitive
- richer gradient / color-over-life helpers

### P3 Cautious
- host-owned post-process passes
- controlled blur / bloom / afterimage / feedback style effects

## v3 P1 Detailed Proposal

### 1. Retained Emitter
Proposed new public commands:
- `upsert_particle_emitter`
- `remove_particle_emitter`

Purpose:
- support hold auras, hover rings, continuous sprays, exhaust, cursor-following particle fields, and persistent sparkle trails
- reduce the need for WASM to keep re-emitting large instant batches every frame just to maintain continuity

Suggested fields:
- `emitter_id`
- `anchor_mode`
  - `absolute_screen`
  - `follow_cursor`
  - `follow_input_point`
- `particle_kind`
  - `glow`
  - `sprite`
- `image_id`
- `emission_rate_per_sec`
- `burst_count`
- `max_particles`
- `direction_rad`
- `spread_rad`
- `speed_min` / `speed_max`
- `accel_x` / `accel_y`
- `drag`
- `size_min_px` / `size_max_px`
- `rotation_min_rad` / `rotation_max_rad`
- `alpha_min` / `alpha_max`
- `color_start_argb` / `color_end_argb`
- `particle_life_ms`
- `emitter_ttl_ms`
- `blend_mode`
- `sort_key`
- `group_id`

Note:
- the current v2 `upsert_glow_emitter/remove_glow_emitter` path has now landed as the Windows + macOS retained glow-emitter baseline
- v3 still needs to generalize that baseline into a reusable retained particle / sprite emitter contract instead of staying glow-only

### 2. Path / Vector Primitive
Proposed new public commands:
- `spawn_path_stroke`
- `spawn_path_fill`

Purpose:
- let WASM organize lightning, ribbons, filled outlines, smooth contours, emblems, and filled polygonal effects directly
- solve the current `spawn_polyline` limitation where only stroked polylines are available

Suggested data model:
- header
- `node_count`
- `nodes[]`

Node opcodes:
- `move_to`
- `line_to`
- `quad_to`
- `cubic_to`
- `close`

Suggested shared style fields:
- `stroke_width_px`
- `stroke_argb`
- `fill_argb`
- `glow_argb`
- `alpha`
- `line_join`
- `line_cap`
- `fill_rule`
- `delay_ms`
- `life_ms`
- `blend_mode`
- `sort_key`
- `group_id`

### 3. Shared Layer / Blend / Clip Contract
v3 should not keep scaling on top of isolated `screen_blend` flags.

Proposed common render semantics for all new commands:
- `blend_mode`
  - `normal`
  - `add`
  - `screen`
- `sort_key`
- `group_id`
- `clip_rect`
- `anchor_mode`

Purpose:
- give different command families stable stacking and composition behavior
- reserve a clean path for future group-level post-processing

Current bridge status:
- the minimal `blend_mode/sort_key/group_id` tail has already landed on selected v2 commands: `spawn_glow_batch`, `spawn_sprite_batch`, and `upsert_glow_emitter`
- current host interpretation is intentionally narrow: `screen|add` both map to screen-like composition, `sort_key` maps to Windows/macOS stacking, and `group_id` is now the retained lifecycle key consumed by `remove_group`; broader group-owned passes are still future work

## v3 P2 Proposal

### 1. Textured Quad / Atlas Primitive
Proposed command:
- `spawn_quad_batch`

Goal:
- support atlas animation, quad distortion, card-flip style sprites, textured debris, and strip-like textured effects

Suggested fields:
- `quad_count`
- `image_id` / `atlas_frame`
- `position`
- `size`
- `uv_rect`
- `rotation`
- `alpha`
- `tint`
- `delay_ms`
- `life_ms`
- `blend_mode`
- `sort_key`

### 2. Ribbon / Trail Strip Primitive
Proposed command:
- `spawn_ribbon_strip`
- or `upsert_ribbon_trail`

Goal:
- express ribbons, energy streaks, light trails, and swing traces through one reusable public surface

## v3 P3 Proposal

### Host-Owned Post Process
If bloom, blur, afterimage, or feedback become necessary, expose only limited host-owned passes:
- `apply_group_blur`
- `apply_group_bloom`
- `apply_group_afterimage`

Constraints:
- plugins only request a controlled pass on a host-owned group
- plugins do not upload arbitrary shaders
- plugins do not allocate or control raw GPU resources

## Compatibility and Migration

### Versioning
- keep v2 stable and supported
- introduce v3 through `api_version=3`
- do not infer v2 plugins as v3 through hidden compatibility rules

### Host Rollout Order
1. add v3 parser, diagnostics, and capability reporting
2. promote retained emitter to a formal Windows + macOS capability
3. add path/vector support
4. add shared layer/blend/clip semantics
5. only then expand into quad/ribbon/post-process

### Template Rollout Order
1. keep the existing v2 template entry
2. add a v3 entry and sample matrix
3. switch the default template to v3 only after both Windows and macOS pass regression gates

## Validation Requirements
Every new public primitive must ship with:
- Windows + macOS implementations
- catalog/schema/state capability exposure
- positive samples
- negative fixtures
- budget truncation / timeout / fallback checks
- documentation and troubleshooting guidance

## Recommended First Implementation Slice
Start with `v3-phase1-emitter`:
1. completed: promote the current `upsert_glow_emitter/remove_glow_emitter` path into a formal cross-platform retained glow-emitter baseline
2. completed: add the minimal shared fields `blend_mode`, `sort_key`, and `group_id` on selected v2 commands through an optional render-semantics tail
3. next: add 2 official samples:
- `hold-aura-field`
- `hover-sprite-fountain`
4. next: add regression gates for:
- emitter lifecycle
- emitter anchor following
- emitter budget limits

Why this first:
- highest user-facing payoff
- lowest disruption to existing v2 commands
- immediately moves the wasm route from burst-dominant effects toward sustained effects

## Current Decision
- v2 remains the current formal public surface
- this document is the high-value design entry for follow-up implementation and review
- the minimal retained glow-emitter baseline now exists on both Windows and macOS, but the generic retained-emitter contract remains follow-up v3 work
