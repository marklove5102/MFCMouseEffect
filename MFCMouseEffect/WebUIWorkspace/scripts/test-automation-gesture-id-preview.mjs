import assert from 'node:assert/strict';

import {
  buildGesturePreviewFromId,
  gestureIdDirectionHint,
  parseGestureDirections,
} from '../src/automation/gesture-id-preview.js';

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

runTest('parse supports canonical diagonal chain ids', () => {
  assert.deepEqual(
    parseGestureDirections('down_diag_down_right_diag_up_right'),
    ['down', 'diag_down_right', 'diag_up_right'],
  );
});

runTest('parse supports alias ids from preset names', () => {
  assert.deepEqual(parseGestureDirections('v'), ['diag_down_right', 'diag_up_right']);
  assert.deepEqual(parseGestureDirections('slash'), ['diag_down_right']);
});

runTest('direction hint renders arrow sequence', () => {
  assert.equal(gestureIdDirectionHint('diag_down_right_diag_up_right'), '↘↗');
  assert.equal(gestureIdDirectionHint('right'), '→');
});

runTest('preview model contains path/start/arrow for valid id', () => {
  const preview = buildGesturePreviewFromId('diag_down_right_diag_up_right');
  assert.ok(preview);
  assert.ok(preview.path.length > 0);
  assert.ok(preview.startPoint);
  assert.ok(preview.arrowPath.length > 0);
});

runTest('preview returns null for unknown id', () => {
  assert.equal(buildGesturePreviewFromId('unknown_shape_id'), null);
});

if (failed > 0) {
  console.error(`[result] failed: ${failed}`);
  process.exit(1);
}

console.log('[result] gesture id preview tests passed');

