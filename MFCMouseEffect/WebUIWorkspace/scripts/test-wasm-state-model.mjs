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
  assert.equal(state.lifetime_throttled_render_commands, 0);
  assert.equal(state.lifetime_dropped_render_commands, 0);
});

if (failed > 0) {
  console.error(`[result] failed: ${failed}`);
  process.exit(1);
}

console.log('[result] wasm state model tests passed');
