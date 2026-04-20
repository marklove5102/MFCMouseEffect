import assert from 'node:assert/strict';

import { normalizeCatalogEntries } from '../src/automation/app-catalog.js';
import {
  normalizeAutomationPayload,
  parseAppScopes,
  readMappings,
  normalizeActions,
  normalizeEditorActions,
  serializeAppScopes,
} from '../src/automation/model.js';
import {
  defaultProcessSuffix,
  normalizeRuntimePlatform,
} from '../src/automation/platform.js';

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

runTest('platform aliases normalize to canonical values', () => {
  assert.equal(normalizeRuntimePlatform('darwin'), 'macos');
  assert.equal(normalizeRuntimePlatform('mac'), 'macos');
  assert.equal(normalizeRuntimePlatform('win32'), 'windows');
  assert.equal(normalizeRuntimePlatform('linux'), 'linux');
});

runTest('platform default suffix matches runtime semantics', () => {
  assert.equal(defaultProcessSuffix('macos'), 'app');
  assert.equal(defaultProcessSuffix('windows'), 'exe');
  assert.equal(defaultProcessSuffix('linux'), '');
});

runTest('catalog normalization applies platform-specific suffix', () => {
  const macEntries = normalizeCatalogEntries([{ exe: 'Code' }], 'macos');
  const windowsEntries = normalizeCatalogEntries([{ exe: 'Code' }], 'windows');

  assert.equal(macEntries[0].exe, 'code.app');
  assert.equal(windowsEntries[0].exe, 'code.exe');
});

runTest('scope parse/serialize uses platform-specific process naming', () => {
  const parsed = parseAppScopes('process:Code', 'macos');
  assert.equal(parsed.mode, 'selected');
  assert.deepEqual(parsed.apps, ['code.app']);

  const serialized = serializeAppScopes('selected', ['Code'], 'macos');
  assert.deepEqual(serialized, ['process:code.app']);
});

runTest('automation payload normalizes schema platform aliases', () => {
  const schema = {
    capabilities: { platform: 'darwin' },
    automation_mouse_actions: [{ value: 'left_click', label: 'Left Click' }],
    automation_app_scopes: [
      { value: 'all', label: 'All Apps' },
      { value: 'selected', label: 'Selected Apps (Multi)' },
    ],
    automation_gesture_patterns: [{ value: 'up', label: 'Up' }],
    automation_gesture_buttons: [{ value: 'right', label: 'Right Drag' }],
  };
  const payload = {
    enabled: true,
    mouse_mappings: [
      {
        enabled: true,
        trigger: 'left_click',
        app_scopes: ['process:Code'],
        actions: [{ type: 'send_shortcut', shortcut: 'Cmd+C' }],
      },
    ],
    gesture: {
      enabled: false,
      mappings: [],
    },
  };

  const normalized = normalizeAutomationPayload(schema, payload);
  assert.equal(normalized.platform, 'macos');
  assert.deepEqual(normalized.mouseMappings[0].appScopeApps, ['code.app']);
});

runTest('readMappings writes platform-correct app_scopes/app_scope', () => {
  const rows = [
    {
      enabled: true,
      triggerChain: ['left_click'],
      appScopeMode: 'selected',
      appScopeApps: ['Code'],
      actions: [{ type: 'send_shortcut', shortcut: 'Cmd+C' }],
    },
  ];
  const options = [{ value: 'left_click', label: 'Left Click' }];
  const mappings = readMappings(rows, options, 'left_click', 'macos');

  assert.equal(mappings.length, 1);
  assert.equal(mappings[0].app_scope, 'process:code.app');
  assert.deepEqual(mappings[0].app_scopes, ['process:code.app']);
  assert.deepEqual(mappings[0].actions, [{ type: 'send_shortcut', shortcut: 'Cmd+C' }]);
});

runTest('automation actions normalize shortcut and delay steps', () => {
  assert.deepEqual(
    normalizeActions([
      { type: 'send_shortcut', shortcut: 'Cmd+C' },
      { type: 'delay', delay_ms: 120 },
      { type: 'open_url', url: 'https://example.com/mfx-open-url' },
      { type: 'launch_app', app_path: '/Applications/Safari.app' },
      { type: 'delay', delay_ms: 999999 },
      { type: 'unknown', shortcut: 'Cmd+X' },
    ]),
    [
      { type: 'send_shortcut', shortcut: 'Cmd+C' },
      { type: 'delay', delay_ms: 120 },
      { type: 'open_url', url: 'https://example.com/mfx-open-url' },
      { type: 'launch_app', app_path: '/Applications/Safari.app' },
      { type: 'delay', delay_ms: 60000 },
    ]);
});

runTest('editor actions keep incomplete steps until save-time normalization', () => {
  assert.deepEqual(
    normalizeEditorActions([
      { type: 'send_shortcut', shortcut: '' },
      { type: 'delay', delay_ms: 0 },
      { type: 'open_url', url: '' },
      { type: 'launch_app', app_path: '' },
    ]),
    [
      { type: 'send_shortcut', shortcut: '' },
      { type: 'delay', delay_ms: '' },
      { type: 'open_url', url: '' },
      { type: 'launch_app', app_path: '' },
    ]);
});

runTest('readMappings keeps open_url actions during roundtrip', () => {
  const rows = [
    {
      enabled: true,
      triggerChain: ['left_click'],
      appScopeMode: 'all',
      actions: [{ type: 'open_url', url: 'https://example.com/mfx-roundtrip' }],
    },
  ];
  const options = [{ value: 'left_click', label: 'Left Click' }];
  const mappings = readMappings(rows, options, 'left_click', 'macos');

  assert.deepEqual(mappings[0].actions, [
    { type: 'open_url', url: 'https://example.com/mfx-roundtrip' },
  ]);
});

if (failed > 0) {
  console.error(`[result] failed: ${failed}`);
  process.exit(1);
}

console.log('[result] all automation platform tests passed');
