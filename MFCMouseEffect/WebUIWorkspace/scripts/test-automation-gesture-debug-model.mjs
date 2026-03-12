import assert from 'node:assert/strict';

import {
  boolLabel,
  formatGestureRouteModifiers,
  gestureRouteSourceLabel,
  normalizeGestureRouteStatus,
} from '../src/automation/gesture-route-debug-model.js';

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

runTest('normalize returns null when gesture debug payload is missing', () => {
  assert.equal(normalizeGestureRouteStatus(undefined), null);
  assert.equal(normalizeGestureRouteStatus({}), null);
  assert.equal(normalizeGestureRouteStatus({ input_automation_gesture_route_status: 1 }), null);
});

runTest('normalize maps gesture route payload fields', () => {
  const normalized = normalizeGestureRouteStatus({
    input_automation_gesture_route_status: {
      automation_enabled: true,
      gesture_enabled: true,
      buttonless_gesture_enabled: false,
      pointer_button_down: true,
      gesture_mapping_count: 3,
      buttonless_gesture_mapping_count: 1,
      last_stage: 'gesture_trigger',
      last_reason: 'preset_similarity_binding_injected',
      last_gesture_id: 'v',
      last_trigger_button: 'right',
      last_matched: true,
      last_injected: true,
      last_used_custom: false,
      last_used_preset: true,
      last_sample_point_count: 42,
      last_modifiers: {
        primary: true,
        shift: false,
        alt: true,
      },
    },
  });

  assert.equal(normalized.automationEnabled, true);
  assert.equal(normalized.gestureEnabled, true);
  assert.equal(normalized.pointerButtonDown, true);
  assert.equal(normalized.gestureMappingCount, 3);
  assert.equal(normalized.buttonlessGestureMappingCount, 1);
  assert.equal(normalized.lastStage, 'gesture_trigger');
  assert.equal(normalized.lastReason, 'preset_similarity_binding_injected');
  assert.equal(normalized.lastGestureId, 'v');
  assert.equal(normalized.lastTriggerButton, 'right');
  assert.equal(normalized.lastMatched, true);
  assert.equal(normalized.lastInjected, true);
  assert.equal(normalized.lastUsedPreset, true);
  assert.equal(normalized.lastSamplePointCount, 42);
  assert.deepEqual(normalized.modifiers, { primary: true, shift: false, alt: true });
});

runTest('modifier formatter uses platform-specific primary/alt labels', () => {
  assert.equal(
    formatGestureRouteModifiers({ primary: true, shift: true, alt: true }, 'macos'),
    'Cmd+Shift+Option');
  assert.equal(
    formatGestureRouteModifiers({ primary: true, shift: true, alt: true }, 'windows'),
    'Ctrl+Shift+Alt');
  assert.equal(
    formatGestureRouteModifiers({ primary: false, shift: false, alt: false }, 'windows', { modifierEmpty: '无' }),
    '无');
});

runTest('source label prefers custom, then preset, then unknown', () => {
  assert.equal(
    gestureRouteSourceLabel({ lastUsedCustom: true, lastUsedPreset: true }, { sourceCustom: '自定义' }),
    '自定义');
  assert.equal(
    gestureRouteSourceLabel({ lastUsedCustom: false, lastUsedPreset: true }, { sourcePreset: '预设' }),
    '预设');
  assert.equal(
    gestureRouteSourceLabel({ lastUsedCustom: false, lastUsedPreset: false }, { sourceUnknown: '未命中' }),
    '未命中');
});

runTest('bool label uses localized yes/no text', () => {
  assert.equal(boolLabel(true, { yes: '是', no: '否' }), '是');
  assert.equal(boolLabel(false, { yes: '是', no: '否' }), '否');
});

if (failed > 0) {
  console.error(`[result] failed: ${failed}`);
  process.exit(1);
}

console.log('[result] gesture route debug model tests passed');
