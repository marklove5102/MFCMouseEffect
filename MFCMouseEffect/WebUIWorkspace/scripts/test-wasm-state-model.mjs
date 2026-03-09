import assert from 'node:assert/strict';

import { normalizeWasmState } from '../src/wasm/state-model.js';

let failed = 0;

function runTest(name, fn) {
  try {
    fn();
    console.log(`[pass] ${name}`);
  } catch (error) {
    failed += 1;
    console.error(`[fail] ${name}`);
    console.error(error instanceof Error ? error.stack || error.message : error);
  }
}

runTest('empty input returns zeroed lifetime diagnostics', () => {
  const state = normalizeWasmState({});
  assert.equal(state.lifetime_invoke_calls, 0);
  assert.equal(state.lifetime_invoke_success_calls, 0);
  assert.equal(state.lifetime_invoke_failed_calls, 0);
  assert.equal(state.lifetime_invoke_duration_us, 0);
  assert.equal(state.lifetime_render_dispatches, 0);
  assert.equal(state.lifetime_rendered_by_wasm_dispatches, 0);
  assert.equal(state.lifetime_executed_text_commands, 0);
  assert.equal(state.lifetime_executed_image_commands, 0);
  assert.equal(state.lifetime_executed_pulse_commands, 0);
  assert.equal(state.lifetime_executed_polyline_commands, 0);
  assert.equal(state.lifetime_executed_path_stroke_commands, 0);
  assert.equal(state.lifetime_executed_path_fill_commands, 0);
  assert.equal(state.lifetime_executed_glow_batch_commands, 0);
  assert.equal(state.lifetime_executed_sprite_batch_commands, 0);
  assert.equal(state.lifetime_executed_glow_emitter_commands, 0);
  assert.equal(state.lifetime_executed_sprite_emitter_commands, 0);
  assert.equal(state.lifetime_executed_particle_emitter_commands, 0);
  assert.equal(state.lifetime_executed_ribbon_trail_commands, 0);
  assert.equal(state.lifetime_executed_quad_field_commands, 0);
  assert.equal(state.lifetime_executed_group_remove_commands, 0);
  assert.equal(state.lifetime_executed_group_presentation_commands, 0);
  assert.equal(state.lifetime_executed_group_clip_rect_commands, 0);
  assert.equal(state.lifetime_executed_group_layer_commands, 0);
  assert.equal(state.lifetime_executed_group_transform_commands, 0);
  assert.equal(state.lifetime_throttled_render_commands, 0);
  assert.equal(state.lifetime_throttled_by_capacity_render_commands, 0);
  assert.equal(state.lifetime_throttled_by_interval_render_commands, 0);
  assert.equal(state.lifetime_dropped_render_commands, 0);
});

runTest('normalizes lifetime diagnostics from numeric input', () => {
  const state = normalizeWasmState({
    lifetime_invoke_calls: 12,
    lifetime_invoke_success_calls: 10,
    lifetime_invoke_failed_calls: 2,
    lifetime_invoke_duration_us: 9411,
    lifetime_invoke_exceeded_budget_calls: 3,
    lifetime_invoke_rejected_by_budget_calls: 1,
    lifetime_render_dispatches: 8,
    lifetime_rendered_by_wasm_dispatches: 6,
    lifetime_executed_text_commands: 32,
    lifetime_executed_image_commands: 14,
    lifetime_executed_pulse_commands: 5,
    lifetime_executed_polyline_commands: 7,
    lifetime_executed_path_stroke_commands: 3,
    lifetime_executed_path_fill_commands: 2,
    lifetime_executed_glow_batch_commands: 9,
    lifetime_executed_sprite_batch_commands: 4,
    lifetime_executed_glow_emitter_commands: 2,
    lifetime_executed_sprite_emitter_commands: 3,
    lifetime_executed_particle_emitter_commands: 5,
    lifetime_executed_particle_emitter_remove_commands: 1,
    lifetime_executed_ribbon_trail_commands: 2,
    lifetime_executed_ribbon_trail_remove_commands: 1,
    lifetime_executed_quad_field_commands: 3,
    lifetime_executed_quad_field_remove_commands: 1,
    lifetime_executed_group_remove_commands: 2,
    lifetime_executed_group_presentation_commands: 4,
    lifetime_executed_group_clip_rect_commands: 3,
    lifetime_executed_group_layer_commands: 5,
    lifetime_executed_group_transform_commands: 6,
    retained_ribbon_trail_upsert_requests: 4,
    retained_ribbon_trail_remove_requests: 1,
    retained_ribbon_trail_active_count: 1,
    retained_quad_field_upsert_requests: 2,
    retained_quad_field_remove_requests: 1,
    retained_quad_field_active_count: 1,
    lifetime_throttled_render_commands: 5,
    lifetime_throttled_by_capacity_render_commands: 3,
    lifetime_throttled_by_interval_render_commands: 2,
    lifetime_dropped_render_commands: 1,
  });

  assert.equal(state.lifetime_invoke_calls, 12);
  assert.equal(state.lifetime_invoke_success_calls, 10);
  assert.equal(state.lifetime_invoke_failed_calls, 2);
  assert.equal(state.lifetime_invoke_duration_us, 9411);
  assert.equal(state.lifetime_invoke_exceeded_budget_calls, 3);
  assert.equal(state.lifetime_invoke_rejected_by_budget_calls, 1);
  assert.equal(state.lifetime_render_dispatches, 8);
  assert.equal(state.lifetime_rendered_by_wasm_dispatches, 6);
  assert.equal(state.lifetime_executed_text_commands, 32);
  assert.equal(state.lifetime_executed_image_commands, 14);
  assert.equal(state.lifetime_executed_pulse_commands, 5);
  assert.equal(state.lifetime_executed_polyline_commands, 7);
  assert.equal(state.lifetime_executed_path_stroke_commands, 3);
  assert.equal(state.lifetime_executed_path_fill_commands, 2);
  assert.equal(state.lifetime_executed_glow_batch_commands, 9);
  assert.equal(state.lifetime_executed_sprite_batch_commands, 4);
  assert.equal(state.lifetime_executed_glow_emitter_commands, 2);
  assert.equal(state.lifetime_executed_sprite_emitter_commands, 3);
  assert.equal(state.lifetime_executed_particle_emitter_commands, 5);
  assert.equal(state.lifetime_executed_particle_emitter_remove_commands, 1);
  assert.equal(state.lifetime_executed_ribbon_trail_commands, 2);
  assert.equal(state.lifetime_executed_ribbon_trail_remove_commands, 1);
  assert.equal(state.lifetime_executed_quad_field_commands, 3);
  assert.equal(state.lifetime_executed_quad_field_remove_commands, 1);
  assert.equal(state.lifetime_executed_group_remove_commands, 2);
  assert.equal(state.lifetime_executed_group_presentation_commands, 4);
  assert.equal(state.lifetime_executed_group_clip_rect_commands, 3);
  assert.equal(state.lifetime_executed_group_layer_commands, 5);
  assert.equal(state.lifetime_executed_group_transform_commands, 6);
  assert.equal(state.retained_ribbon_trail_upsert_requests, 4);
  assert.equal(state.retained_ribbon_trail_remove_requests, 1);
  assert.equal(state.retained_ribbon_trail_active_count, 1);
  assert.equal(state.retained_quad_field_upsert_requests, 2);
  assert.equal(state.retained_quad_field_remove_requests, 1);
  assert.equal(state.retained_quad_field_active_count, 1);
  assert.equal(state.lifetime_throttled_render_commands, 5);
  assert.equal(state.lifetime_throttled_by_capacity_render_commands, 3);
  assert.equal(state.lifetime_throttled_by_interval_render_commands, 2);
  assert.equal(state.lifetime_dropped_render_commands, 1);
});

runTest('invalid lifetime diagnostics fallback to zero', () => {
  const state = normalizeWasmState({
    lifetime_invoke_calls: 'bad',
    lifetime_invoke_success_calls: null,
    lifetime_invoke_failed_calls: undefined,
    lifetime_invoke_duration_us: NaN,
    lifetime_render_dispatches: 'x',
    lifetime_executed_text_commands: -1,
    lifetime_executed_image_commands: '',
    lifetime_executed_pulse_commands: 'bad',
    lifetime_executed_polyline_commands: null,
    lifetime_executed_path_stroke_commands: undefined,
    lifetime_executed_path_fill_commands: undefined,
    lifetime_executed_glow_batch_commands: undefined,
    lifetime_executed_sprite_batch_commands: {},
    lifetime_executed_glow_emitter_commands: null,
    lifetime_executed_sprite_emitter_commands: undefined,
    lifetime_executed_particle_emitter_commands: undefined,
    lifetime_executed_ribbon_trail_commands: undefined,
    lifetime_executed_quad_field_commands: undefined,
    lifetime_executed_group_remove_commands: undefined,
    lifetime_executed_group_presentation_commands: undefined,
    lifetime_executed_group_clip_rect_commands: undefined,
    lifetime_executed_group_layer_commands: undefined,
    lifetime_executed_group_transform_commands: undefined,
    lifetime_throttled_render_commands: {},
    lifetime_dropped_render_commands: [],
  });

  assert.equal(state.lifetime_invoke_calls, 0);
  assert.equal(state.lifetime_invoke_success_calls, 0);
  assert.equal(state.lifetime_invoke_failed_calls, 0);
  assert.equal(state.lifetime_invoke_duration_us, 0);
  assert.equal(state.lifetime_render_dispatches, 0);
  assert.equal(state.lifetime_executed_text_commands, -1);
  assert.equal(state.lifetime_executed_image_commands, 0);
  assert.equal(state.lifetime_executed_pulse_commands, 0);
  assert.equal(state.lifetime_executed_polyline_commands, 0);
  assert.equal(state.lifetime_executed_path_stroke_commands, 0);
  assert.equal(state.lifetime_executed_path_fill_commands, 0);
  assert.equal(state.lifetime_executed_glow_batch_commands, 0);
  assert.equal(state.lifetime_executed_sprite_batch_commands, 0);
  assert.equal(state.lifetime_executed_glow_emitter_commands, 0);
  assert.equal(state.lifetime_executed_sprite_emitter_commands, 0);
  assert.equal(state.lifetime_executed_particle_emitter_commands, 0);
  assert.equal(state.lifetime_executed_ribbon_trail_commands, 0);
  assert.equal(state.lifetime_executed_quad_field_commands, 0);
  assert.equal(state.lifetime_executed_group_remove_commands, 0);
  assert.equal(state.lifetime_executed_group_presentation_commands, 0);
  assert.equal(state.lifetime_executed_group_clip_rect_commands, 0);
  assert.equal(state.lifetime_executed_group_layer_commands, 0);
  assert.equal(state.lifetime_executed_group_transform_commands, 0);
  assert.equal(state.lifetime_throttled_render_commands, 0);
  assert.equal(state.lifetime_dropped_render_commands, 0);
});

if (failed > 0) {
  console.error(`[result] failed: ${failed}`);
  process.exit(1);
}

console.log('[result] wasm state model tests passed');
