import assert from 'node:assert/strict';

import {
  boolLabel,
  formatGestureRouteModifiers,
  gestureRouteSourceLabel,
  normalizeGestureRouteStatus,
  recentGestureRouteEvents,
  selectLatestGestureEvent,
  selectLatestMatchedGestureEvent,
  selectLatestRecognizedGestureEvent,
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
      last_recognized_gesture_id: 'w',
      last_matched_gesture_id: 'v',
      last_trigger_button: 'right',
      last_matched: true,
      last_injected: true,
      last_used_custom: false,
      last_used_preset: true,
      last_sample_point_count: 42,
      last_candidate_count: 9,
      last_best_window_start: 5,
      last_best_window_end: 27,
      last_runner_up_score: 71.2,
      last_event_seq: 12,
      last_modifiers: {
        primary: true,
        shift: false,
        alt: true,
      },
      recent_events: [
        {
          seq: 11,
          timestamp_ms: 1010,
          stage: 'gesture_trigger',
          reason: 'preset_similarity_binding_not_matched',
          gesture_id: 'w',
          recognized_gesture_id: 'w',
          matched_gesture_id: '',
          trigger_button: 'right',
          matched: false,
          injected: false,
          used_custom: false,
          used_preset: true,
          sample_point_count: 28,
          candidate_count: 4,
          best_window_start: 1,
          best_window_end: 14,
          runner_up_score: 66.6,
          modifiers: {
            primary: false,
            shift: false,
            alt: false,
          },
        },
        {
          seq: 12,
          timestamp_ms: 1020,
          stage: 'gesture_trigger',
          reason: 'preset_similarity_binding_injected',
          gesture_id: 'v',
          recognized_gesture_id: 'w',
          matched_gesture_id: 'v',
          trigger_button: 'right',
          matched: true,
          injected: true,
          used_custom: false,
          used_preset: true,
          sample_point_count: 42,
          candidate_count: 9,
          best_window_start: 5,
          best_window_end: 27,
          runner_up_score: 71.2,
          modifiers: {
            primary: true,
            shift: false,
            alt: true,
          },
        },
      ],
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
  assert.equal(normalized.lastRecognizedGestureId, 'w');
  assert.equal(normalized.lastMatchedGestureId, 'v');
  assert.equal(normalized.lastTriggerButton, 'right');
  assert.equal(normalized.lastMatched, true);
  assert.equal(normalized.lastInjected, true);
  assert.equal(normalized.lastUsedPreset, true);
  assert.equal(normalized.lastSamplePointCount, 42);
  assert.equal(normalized.lastCandidateCount, 9);
  assert.equal(normalized.lastBestWindowStart, 5);
  assert.equal(normalized.lastBestWindowEnd, 27);
  assert.equal(normalized.lastRunnerUpScore, 71.2);
  assert.equal(normalized.lastEventSeq, 12);
  assert.deepEqual(normalized.modifiers, { primary: true, shift: false, alt: true });
  assert.equal(normalized.recentEvents.length, 2);
  assert.equal(normalized.recentEvents[1].seq, 12);
  assert.equal(normalized.recentEvents[1].reason, 'preset_similarity_binding_injected');
  assert.equal(normalized.recentEvents[1].recognizedGestureId, 'w');
  assert.equal(normalized.recentEvents[1].matchedGestureId, 'v');
  assert.equal(normalized.recentEvents[1].candidateCount, 9);
  assert.equal(normalized.recentEvents[1].bestWindowStart, 5);
  assert.equal(normalized.recentEvents[1].bestWindowEnd, 27);
  assert.equal(normalized.recentEvents[1].runnerUpScore, 71.2);
  assert.deepEqual(normalized.recentEvents[1].modifiers, { primary: true, shift: false, alt: true });
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

runTest('recent events returns latest N entries in reverse order', () => {
  const normalized = normalizeGestureRouteStatus({
    input_automation_gesture_route_status: {
      recent_events: [
        { seq: 1, stage: 'a', reason: 'r1' },
        { seq: 2, stage: 'b', reason: 'r2' },
        { seq: 3, stage: 'c', reason: 'r3' },
      ],
    },
  });

  const latest = recentGestureRouteEvents(normalized, 2);
  assert.equal(latest.length, 2);
  assert.equal(latest[0].seq, 3);
  assert.equal(latest[1].seq, 2);
});

runTest('latest gesture event skips blank gesture ids', () => {
  const normalized = normalizeGestureRouteStatus({
    input_automation_gesture_route_status: {
      recent_events: [
        { seq: 10, stage: 'gesture_trigger', reason: 'empty', gesture_id: '' },
        { seq: 11, stage: 'gesture_trigger', reason: 'ok', gesture_id: 'diag_down_right_diag_up_right' },
        { seq: 12, stage: 'buttonless_move_skipped', reason: 'buttonless_disabled', gesture_id: '' },
      ],
    },
  });
  const event = selectLatestGestureEvent(normalized);
  assert.ok(event);
  assert.equal(event.seq, 11);
  assert.equal(event.gestureId, 'diag_down_right_diag_up_right');
});

runTest('latest recognized and matched events read dedicated gesture ids', () => {
  const normalized = normalizeGestureRouteStatus({
    input_automation_gesture_route_status: {
      recent_events: [
        { seq: 20, stage: 'gesture_drag_snapshot', reason: 'collecting', recognized_gesture_id: 'v', matched_gesture_id: '' },
        { seq: 21, stage: 'gesture_trigger', reason: 'preset_window_injected', recognized_gesture_id: 'w', matched_gesture_id: 'w' },
        { seq: 22, stage: 'buttonless_move_skipped', reason: 'buttonless_disabled', recognized_gesture_id: '', matched_gesture_id: '' },
      ],
    },
  });

  const recognized = selectLatestRecognizedGestureEvent(normalized);
  const matched = selectLatestMatchedGestureEvent(normalized);
  assert.ok(recognized);
  assert.ok(matched);
  assert.equal(recognized.seq, 21);
  assert.equal(recognized.recognizedGestureId, 'w');
  assert.equal(matched.seq, 21);
  assert.equal(matched.matchedGestureId, 'w');
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
