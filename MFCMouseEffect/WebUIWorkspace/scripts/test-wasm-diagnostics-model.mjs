import {
  buildWasmLifetimeInvokeText,
  buildWasmLifetimeRenderText,
} from '../src/wasm/diagnostics-model.js';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function escapeRegex(value) {
  return `${value || ''}`.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function extractObjectBody(source, marker) {
  const markerIndex = source.indexOf(marker);
  assert(markerIndex >= 0, `i18n marker not found: ${marker}`);

  const braceStart = source.indexOf('{', markerIndex);
  assert(braceStart >= 0, `i18n object start not found: ${marker}`);

  let depth = 0;
  for (let i = braceStart; i < source.length; i += 1) {
    const ch = source[i];
    if (ch === '{') {
      depth += 1;
      continue;
    }
    if (ch !== '}') {
      continue;
    }
    depth -= 1;
    if (depth === 0) {
      return source.slice(braceStart + 1, i);
    }
  }
  throw new Error(`i18n object end not found: ${marker}`);
}

function resolveWebUiI18nPath() {
  const scriptDir = path.dirname(fileURLToPath(import.meta.url));
  const workspaceDir = path.resolve(scriptDir, '..');
  const projectDir = path.resolve(workspaceDir, '..');
  return path.join(projectDir, 'WebUI', 'i18n.js');
}

function testBuildLifetimeInvokeTextFallback() {
  const text = buildWasmLifetimeInvokeText({
    lifetime_invoke_calls: 12,
    lifetime_invoke_success_calls: 10,
    lifetime_invoke_failed_calls: 2,
    lifetime_invoke_exceeded_budget_calls: 1,
    lifetime_invoke_rejected_by_budget_calls: 3,
  });
  assert(text.includes('Total=12'), 'lifetime invoke text should include total');
  assert(text.includes('Success=10'), 'lifetime invoke text should include success');
  assert(text.includes('Failed=2'), 'lifetime invoke text should include failed');
  assert(text.includes('ExceededBudget=1'), 'lifetime invoke text should include exceeded budget');
  assert(text.includes('RejectedBudget=3'), 'lifetime invoke text should include rejected budget');
}

function testBuildLifetimeRenderTextFallback() {
  const text = buildWasmLifetimeRenderText({
    lifetime_render_dispatches: 9,
    lifetime_rendered_by_wasm_dispatches: 8,
    lifetime_executed_text_commands: 31,
    lifetime_executed_image_commands: 17,
    lifetime_throttled_render_commands: 7,
    lifetime_throttled_by_capacity_render_commands: 4,
    lifetime_throttled_by_interval_render_commands: 3,
    lifetime_dropped_render_commands: 2,
  });
  assert(text.includes('Total=9'), 'lifetime render text should include total');
  assert(text.includes('Rendered=8'), 'lifetime render text should include rendered');
  assert(text.includes('TextCmd=31'), 'lifetime render text should include text commands');
  assert(text.includes('ImgCmd=17'), 'lifetime render text should include image commands');
  assert(text.includes('Throttled=7 (cap=4, int=3)'), 'lifetime render text should include throttle split');
  assert(text.includes('Dropped=2'), 'lifetime render text should include dropped commands');
}

function testBuildLifetimeTextWithTranslate() {
  const t = (key, fallback) => {
    if (key === 'label_wasm_metric_total') return 'SUM';
    if (key === 'label_wasm_metric_success') return 'OK';
    return fallback;
  };
  const text = buildWasmLifetimeInvokeText({
    lifetime_invoke_calls: 2,
    lifetime_invoke_success_calls: 1,
    lifetime_invoke_failed_calls: 1,
  }, t);
  assert(text.includes('SUM=2'), 'translated total label should be used');
  assert(text.includes('OK=1'), 'translated success label should be used');
}

function testI18nKeyParity() {
  const i18nPath = resolveWebUiI18nPath();
  const source = fs.readFileSync(i18nPath, 'utf8');
  const enSection = extractObjectBody(source, '"en-US": {');
  const zhSection = extractObjectBody(source, '"zh-CN": {');
  const keys = [
    'label_wasm_lifetime_invoke_stats',
    'label_wasm_lifetime_render_stats',
    'label_wasm_metric_total',
    'label_wasm_metric_success',
    'label_wasm_metric_failed',
    'label_wasm_metric_rendered',
    'label_wasm_metric_text_commands',
    'label_wasm_metric_image_commands',
    'label_wasm_metric_throttled',
    'label_wasm_metric_dropped',
    'label_wasm_metric_exceeded_budget',
    'label_wasm_metric_rejected_budget',
  ];

  for (const key of keys) {
    const keyPattern = new RegExp(`\\b${escapeRegex(key)}\\s*:`);
    assert(keyPattern.test(enSection), `missing i18n key in en-US: ${key}`);
    assert(keyPattern.test(zhSection), `missing i18n key in zh-CN: ${key}`);
  }
}

function main() {
  testBuildLifetimeInvokeTextFallback();
  testBuildLifetimeRenderTextFallback();
  testBuildLifetimeTextWithTranslate();
  testI18nKeyParity();
  console.log('[result] wasm diagnostics model tests passed');
}

main();
