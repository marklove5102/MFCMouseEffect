import {
  MOUSE_COMPANION_CHECKBOX_FIELDS,
  MOUSE_COMPANION_DEFAULT_ACTIVE_TAB,
  MOUSE_COMPANION_FLOAT_FIELDS,
  MOUSE_COMPANION_NUMBER_FIELDS,
  MOUSE_COMPANION_RANGE_BINDINGS,
  MOUSE_COMPANION_TAB_IDS,
  MOUSE_COMPANION_TEXT_FIELDS,
} from '../mouse-companion/form-contract.js';
import {
  MOUSE_COMPANION_DEFAULT_RUNTIME_STATE,
  normalizeMouseCompanionProbeRuntimeResponse,
  normalizeMouseCompanionRuntimeState,
  writeMouseCompanionRuntimeStateToDom,
} from '../mouse-companion/runtime-diagnostics.js';
import { createMouseCompanionProbeController } from '../mouse-companion/probe-controller.js';
import { readUiState, writeUiState } from './ui-state-storage.js';
import { getMouseCompanionSectionMarkup } from '../mouse-companion/section-template.js';
import { createMouseCompanionTabController } from '../mouse-companion/tab-controller.js';

const MOUSE_COMPANION_UI_STATE_STORAGE_NS = 'mouse-companion.v1';

const DEFAULT_SCHEMA = {
  position_modes: [
    { value: 'fixed_bottom_left', label: 'Fixed Bottom Left (Recommended For Click Tuning)' },
    { value: 'follow', label: 'Follow Cursor' },
  ],
  edge_clamp_modes: [
    { value: 'soft', label: 'Soft Edge (Recommended)' },
    { value: 'free', label: 'Free (No Clamp)' },
    { value: 'strict', label: 'Strict (In Screen)' },
  ],
  size_px_range: { min: 48, max: 360, step: 1 },
  offset_range: { min: -1200, max: 1200, step: 1 },
  press_lift_px_range: { min: 0, max: 240, step: 1 },
  smoothing_percent_range: { min: 0, max: 95, step: 1 },
  follow_threshold_px_range: { min: 0, max: 32, step: 1 },
  release_hold_ms_range: { min: 0, max: 800, step: 10 },
  click_streak_break_ms_range: { min: 120, max: 3000, step: 10 },
  head_tint_per_click_range: { min: 0.01, max: 1.0, step: 0.01 },
  head_tint_max_range: { min: 0.01, max: 1.0, step: 0.01 },
  head_tint_decay_per_second_range: { min: 0.05, max: 4.0, step: 0.01 },
  test_press_lift_px_range: { min: 0, max: 320, step: 1 },
  test_smoothing_percent_range: { min: 0, max: 95, step: 1 },
  test_click_streak_break_ms_range: { min: 120, max: 3000, step: 10 },
  test_head_tint_per_click_range: { min: 0.01, max: 1.0, step: 0.01 },
  test_head_tint_max_range: { min: 0.01, max: 1.0, step: 0.01 },
  test_head_tint_decay_per_second_range: { min: 0.05, max: 4.0, step: 0.01 },
};

const DEFAULT_STATE = {
  enabled: false,
  face_pointer_enabled: false,
  model_path: 'MFCMouseEffect/Assets/Pet3D/source/pet-main.glb',
  action_library_path: 'MFCMouseEffect/Assets/Pet3D/source/pet-actions.json',
  appearance_profile_path: 'MFCMouseEffect/Assets/Pet3D/source/pet-appearance.json',
  position_mode: 'fixed_bottom_left',
  edge_clamp_mode: 'soft',
  size_px: 112,
  offset_x: 18,
  offset_y: 26,
  press_lift_px: 24,
  smoothing_percent: 68,
  follow_threshold_px: 2,
  release_hold_ms: 120,
  click_streak_break_ms: 650,
  head_tint_per_click: 0.11,
  head_tint_max: 0.7,
  head_tint_decay_per_second: 0.36,
  use_test_profile: false,
  test_press_lift_px: 48,
  test_smoothing_percent: 32,
  test_click_streak_break_ms: 1200,
  test_head_tint_per_click: 0.2,
  test_head_tint_max: 0.8,
  test_head_tint_decay_per_second: 0.15,
};

let latestSchema = { ...DEFAULT_SCHEMA };
let latestState = { ...DEFAULT_STATE };
let latestRuntimeState = { ...MOUSE_COMPANION_DEFAULT_RUNTIME_STATE };

function byId(id) {
  return document.getElementById(id);
}

function readMouseCompanionUiState() {
  return readUiState(MOUSE_COMPANION_UI_STATE_STORAGE_NS);
}

function writeMouseCompanionUiState(nextState) {
  writeUiState(MOUSE_COMPANION_UI_STATE_STORAGE_NS, nextState);
}

const tabController = createMouseCompanionTabController({
  byId,
  tabIds: MOUSE_COMPANION_TAB_IDS,
  defaultTabId: MOUSE_COMPANION_DEFAULT_ACTIVE_TAB,
  initialActiveTab: readMouseCompanionUiState()?.activeTab,
  onActiveTabChange: (activeTab) => {
    writeMouseCompanionUiState({ activeTab });
  },
});

function clampInt(value, min, max, fallback) {
  const parsed = Number.parseInt(`${value ?? ''}`, 10);
  const safe = Number.isFinite(parsed) ? parsed : fallback;
  return Math.min(max, Math.max(min, safe));
}

function clampNumber(value, min, max, fallback) {
  const parsed = Number.parseFloat(`${value ?? ''}`);
  const safe = Number.isFinite(parsed) ? parsed : fallback;
  return Math.min(max, Math.max(min, safe));
}

function normalizeRange(value, fallback) {
  const source = value && typeof value === 'object' ? value : {};
  return {
    min: clampNumber(source.min, fallback.min, fallback.max, fallback.min),
    max: clampNumber(source.max, fallback.min, fallback.max, fallback.max),
    step: clampNumber(source.step, 0.001, 1000, fallback.step),
  };
}

function normalizeSchema(value) {
  const source = value && typeof value === 'object' ? value : {};
  const normalizeModeOptions = (input, fallback) => {
    if (!Array.isArray(input)) {
      return [...fallback];
    }
    const normalized = input
      .map((item) => {
        const sourceItem = item && typeof item === 'object' ? item : {};
        const value = `${sourceItem.value ?? ''}`.trim().toLowerCase();
        const label = `${sourceItem.label ?? ''}`.trim();
        if (!value) {
          return null;
        }
        return {
          value,
          label: label || value,
        };
      })
      .filter((item) => !!item);
    if (normalized.length <= 0) {
      return [...fallback];
    }
    return normalized;
  };
  return {
    position_modes: normalizeModeOptions(source.position_modes, DEFAULT_SCHEMA.position_modes),
    edge_clamp_modes: normalizeModeOptions(source.edge_clamp_modes, DEFAULT_SCHEMA.edge_clamp_modes),
    size_px_range: normalizeRange(source.size_px_range, DEFAULT_SCHEMA.size_px_range),
    offset_range: normalizeRange(source.offset_range, DEFAULT_SCHEMA.offset_range),
    press_lift_px_range: normalizeRange(source.press_lift_px_range, DEFAULT_SCHEMA.press_lift_px_range),
    smoothing_percent_range: normalizeRange(source.smoothing_percent_range, DEFAULT_SCHEMA.smoothing_percent_range),
    follow_threshold_px_range: normalizeRange(source.follow_threshold_px_range, DEFAULT_SCHEMA.follow_threshold_px_range),
    release_hold_ms_range: normalizeRange(source.release_hold_ms_range, DEFAULT_SCHEMA.release_hold_ms_range),
    click_streak_break_ms_range: normalizeRange(source.click_streak_break_ms_range, DEFAULT_SCHEMA.click_streak_break_ms_range),
    head_tint_per_click_range: normalizeRange(source.head_tint_per_click_range, DEFAULT_SCHEMA.head_tint_per_click_range),
    head_tint_max_range: normalizeRange(source.head_tint_max_range, DEFAULT_SCHEMA.head_tint_max_range),
    head_tint_decay_per_second_range: normalizeRange(
      source.head_tint_decay_per_second_range,
      DEFAULT_SCHEMA.head_tint_decay_per_second_range,
    ),
    test_press_lift_px_range: normalizeRange(source.test_press_lift_px_range, DEFAULT_SCHEMA.test_press_lift_px_range),
    test_smoothing_percent_range: normalizeRange(source.test_smoothing_percent_range, DEFAULT_SCHEMA.test_smoothing_percent_range),
    test_click_streak_break_ms_range: normalizeRange(
      source.test_click_streak_break_ms_range,
      DEFAULT_SCHEMA.test_click_streak_break_ms_range,
    ),
    test_head_tint_per_click_range: normalizeRange(
      source.test_head_tint_per_click_range,
      DEFAULT_SCHEMA.test_head_tint_per_click_range,
    ),
    test_head_tint_max_range: normalizeRange(
      source.test_head_tint_max_range,
      DEFAULT_SCHEMA.test_head_tint_max_range,
    ),
    test_head_tint_decay_per_second_range: normalizeRange(
      source.test_head_tint_decay_per_second_range,
      DEFAULT_SCHEMA.test_head_tint_decay_per_second_range,
    ),
  };
}

function normalizeState(value) {
  const source = value && typeof value === 'object' ? value : {};
  const modelPath = `${source.model_path || ''}`.trim();
  const actionLibraryPath = `${source.action_library_path || ''}`.trim();
  const appearanceProfilePath = `${source.appearance_profile_path || ''}`.trim();
  const positionMode = `${source.position_mode ?? ''}`.trim().toLowerCase();
  const edgeClampMode = `${source.edge_clamp_mode ?? ''}`.trim().toLowerCase();
  const prodTintPerClick = clampNumber(
    source.head_tint_per_click,
    0.01,
    1.0,
    DEFAULT_STATE.head_tint_per_click,
  );
  const testTintPerClick = clampNumber(
    source.test_head_tint_per_click,
    0.01,
    1.0,
    DEFAULT_STATE.test_head_tint_per_click,
  );
  return {
    enabled: !!source.enabled,
    face_pointer_enabled: !!source.face_pointer_enabled,
    model_path: modelPath || DEFAULT_STATE.model_path,
    action_library_path: actionLibraryPath || DEFAULT_STATE.action_library_path,
    appearance_profile_path: appearanceProfilePath || DEFAULT_STATE.appearance_profile_path,
    position_mode:
      positionMode === 'follow' || positionMode === 'fixed_bottom_left'
        ? positionMode
        : DEFAULT_STATE.position_mode,
    edge_clamp_mode:
      edgeClampMode === 'strict' || edgeClampMode === 'free' || edgeClampMode === 'soft'
        ? edgeClampMode
        : DEFAULT_STATE.edge_clamp_mode,
    size_px: clampInt(source.size_px, 48, 360, DEFAULT_STATE.size_px),
    offset_x: clampInt(source.offset_x, -1200, 1200, DEFAULT_STATE.offset_x),
    offset_y: clampInt(source.offset_y, -1200, 1200, DEFAULT_STATE.offset_y),
    press_lift_px: clampInt(source.press_lift_px, 0, 240, DEFAULT_STATE.press_lift_px),
    smoothing_percent: clampInt(source.smoothing_percent, 0, 95, DEFAULT_STATE.smoothing_percent),
    follow_threshold_px: clampInt(source.follow_threshold_px, 0, 32, DEFAULT_STATE.follow_threshold_px),
    release_hold_ms: clampInt(source.release_hold_ms, 0, 800, DEFAULT_STATE.release_hold_ms),
    click_streak_break_ms: clampInt(
      source.click_streak_break_ms,
      120,
      3000,
      DEFAULT_STATE.click_streak_break_ms,
    ),
    head_tint_per_click: prodTintPerClick,
    head_tint_max: clampNumber(
      source.head_tint_max,
      prodTintPerClick,
      1.0,
      DEFAULT_STATE.head_tint_max,
    ),
    head_tint_decay_per_second: clampNumber(
      source.head_tint_decay_per_second,
      0.05,
      4.0,
      DEFAULT_STATE.head_tint_decay_per_second,
    ),
    use_test_profile: !!source.use_test_profile,
    test_press_lift_px: clampInt(source.test_press_lift_px, 0, 320, DEFAULT_STATE.test_press_lift_px),
    test_smoothing_percent: clampInt(source.test_smoothing_percent, 0, 95, DEFAULT_STATE.test_smoothing_percent),
    test_click_streak_break_ms: clampInt(
      source.test_click_streak_break_ms,
      120,
      3000,
      DEFAULT_STATE.test_click_streak_break_ms,
    ),
    test_head_tint_per_click: testTintPerClick,
    test_head_tint_max: clampNumber(
      source.test_head_tint_max,
      testTintPerClick,
      1.0,
      DEFAULT_STATE.test_head_tint_max,
    ),
    test_head_tint_decay_per_second: clampNumber(
      source.test_head_tint_decay_per_second,
      0.05,
      4.0,
      DEFAULT_STATE.test_head_tint_decay_per_second,
    ),
  };
}

function normalizeRuntimeState(value) {
  return normalizeMouseCompanionRuntimeState(value);
}

function readChecked(id) {
  const node = byId(id);
  return !!(node && node.checked);
}

function readNumber(id, fallback) {
  const node = byId(id);
  if (!node) {
    return fallback;
  }
  return clampInt(node.value, -20000, 20000, fallback);
}

function readFloat(id, fallback) {
  const node = byId(id);
  if (!node) {
    return fallback;
  }
  return clampNumber(node.value, -20000, 20000, fallback);
}

function writeChecked(id, value) {
  const node = byId(id);
  if (!node) {
    return;
  }
  node.checked = !!value;
}

function writeInputValue(id, value) {
  const node = byId(id);
  if (!node) {
    return;
  }
  node.value = `${value ?? ''}`;
}

function applyRange(inputId, range) {
  const node = byId(inputId);
  if (!node || !range) {
    return;
  }
  node.min = `${range.min}`;
  node.max = `${range.max}`;
  node.step = `${range.step}`;
}

const probeController = createMouseCompanionProbeController({
  byId,
  clampInt,
  resolveApiPost: () => {
    const core = window.MfxAppCore;
    if (!core || typeof core.apiPost !== 'function') {
      return null;
    }
    return core.apiPost.bind(core);
  },
  normalizeRuntimeState,
  normalizeProbeRuntimeResponse: normalizeMouseCompanionProbeRuntimeResponse,
  onRuntimeState: (runtimeState) => {
    latestRuntimeState = runtimeState;
    writeRuntimeToDom(latestRuntimeState);
  },
  getRuntimeState: () => latestRuntimeState,
});

function applySelectOptions(selectId, options, selected, fallbackValue, fallbackOptions) {
  const node = byId(selectId);
  if (!node) {
    return;
  }
  const normalized = Array.isArray(options) && options.length > 0
    ? options
    : [...(Array.isArray(fallbackOptions) ? fallbackOptions : [])];
  const selectedValue = `${selected ?? ''}`.trim().toLowerCase();
  node.innerHTML = '';
  for (const option of normalized) {
    const value = `${option?.value ?? ''}`.trim().toLowerCase();
    if (!value) {
      continue;
    }
    const item = document.createElement('option');
    item.value = value;
    item.textContent = `${option?.label ?? value}`;
    node.appendChild(item);
  }
  node.value = selectedValue || `${fallbackValue ?? ''}`;
}

function syncTestFieldState() {
  const useTestProfile = readChecked('mc_use_test_profile');
  const testPressLift = byId('mc_test_press_lift_px');
  const testSmoothing = byId('mc_test_smoothing_percent');
  const testClickStreakBreak = byId('mc_test_click_streak_break_ms');
  const testHeadTintPerClick = byId('mc_test_head_tint_per_click');
  const testHeadTintMax = byId('mc_test_head_tint_max');
  const testHeadTintDecayPerSecond = byId('mc_test_head_tint_decay_per_second');
  if (testPressLift) {
    testPressLift.disabled = !useTestProfile;
  }
  if (testSmoothing) {
    testSmoothing.disabled = !useTestProfile;
  }
  if (testClickStreakBreak) {
    testClickStreakBreak.disabled = !useTestProfile;
  }
  if (testHeadTintPerClick) {
    testHeadTintPerClick.disabled = !useTestProfile;
  }
  if (testHeadTintMax) {
    testHeadTintMax.disabled = !useTestProfile;
  }
  if (testHeadTintDecayPerSecond) {
    testHeadTintDecayPerSecond.disabled = !useTestProfile;
  }

  const textNode = byId('mc_use_test_profile_text');
  if (textNode) {
    textNode.setAttribute(
      'data-i18n',
      useTestProfile ? 'text_mouse_companion_test_on' : 'text_mouse_companion_test_off',
    );
    textNode.textContent = useTestProfile ? 'Testing' : 'Production';
  }
}

function syncFacePointerText() {
  const enabled = readChecked('mc_face_pointer_enabled');
  const textNode = byId('mc_face_pointer_enabled_text');
  if (!textNode) {
    return;
  }
  textNode.setAttribute(
    'data-i18n',
    enabled ? 'text_mouse_companion_face_pointer_on' : 'text_mouse_companion_face_pointer_off',
  );
  textNode.textContent = enabled ? 'Enabled' : 'Disabled';
}

function syncEnabledText() {
  const enabled = readChecked('mc_enabled');
  const textNode = byId('mc_enabled_text');
  if (!textNode) {
    return;
  }
  textNode.setAttribute('data-i18n', enabled ? 'text_mouse_companion_on' : 'text_mouse_companion_off');
  textNode.textContent = enabled ? 'Enabled' : 'Disabled';
}

function writeRuntimeToDom(runtimeState) {
  writeMouseCompanionRuntimeStateToDom(
    runtimeState,
    byId,
    MOUSE_COMPANION_DEFAULT_RUNTIME_STATE,
  );
}

function mountIfNeeded() {
  const mount = byId('mouse_companion_settings_mount');
  if (!mount) {
    return null;
  }
  if (mount.dataset.mfxMouseCompanionMounted === '1') {
    return mount;
  }

  mount.innerHTML = getMouseCompanionSectionMarkup();

  const enabledToggle = byId('mc_enabled');
  if (enabledToggle) {
    enabledToggle.addEventListener('change', syncEnabledText);
  }
  const testToggle = byId('mc_use_test_profile');
  if (testToggle) {
    testToggle.addEventListener('change', syncTestFieldState);
  }
  const facePointerToggle = byId('mc_face_pointer_enabled');
  if (facePointerToggle) {
    facePointerToggle.addEventListener('change', syncFacePointerText);
  }
  tabController.bindTabActions();
  probeController.bindProbeActions();
  tabController.syncTabUi();

  mount.dataset.mfxMouseCompanionMounted = '1';
  return mount;
}

function readFromDom() {
  const draft = {};
  for (const field of MOUSE_COMPANION_CHECKBOX_FIELDS) {
    draft[field.key] = readChecked(field.id);
  }
  for (const field of MOUSE_COMPANION_TEXT_FIELDS) {
    draft[field.key] = `${byId(field.id)?.value || ''}`.trim();
  }
  for (const field of MOUSE_COMPANION_NUMBER_FIELDS) {
    draft[field.key] = readNumber(field.id, DEFAULT_STATE[field.key]);
  }
  for (const field of MOUSE_COMPANION_FLOAT_FIELDS) {
    draft[field.key] = readFloat(field.id, DEFAULT_STATE[field.key]);
  }
  draft.edge_clamp_mode = `${byId('mc_edge_clamp_mode')?.value || ''}`.trim();
  return normalizeState(draft);
}

function writeToDom(state, schema) {
  for (const field of MOUSE_COMPANION_CHECKBOX_FIELDS) {
    writeChecked(field.id, state[field.key]);
  }
  for (const field of MOUSE_COMPANION_TEXT_FIELDS) {
    writeInputValue(field.id, state[field.key] || DEFAULT_STATE[field.key]);
  }
  for (const field of MOUSE_COMPANION_NUMBER_FIELDS) {
    writeInputValue(field.id, state[field.key]);
  }
  for (const field of MOUSE_COMPANION_FLOAT_FIELDS) {
    writeInputValue(field.id, state[field.key]);
  }
  applySelectOptions(
    'mc_position_mode',
    schema.position_modes,
    state.position_mode,
    DEFAULT_STATE.position_mode,
    DEFAULT_SCHEMA.position_modes,
  );
  applySelectOptions(
    'mc_edge_clamp_mode',
    schema.edge_clamp_modes,
    state.edge_clamp_mode,
    DEFAULT_STATE.edge_clamp_mode,
    DEFAULT_SCHEMA.edge_clamp_modes,
  );

  for (const binding of MOUSE_COMPANION_RANGE_BINDINGS) {
    applyRange(binding.id, schema[binding.schemaKey]);
  }
  probeController.writeProbeInput();
  probeController.restoreProbeResult();

  syncEnabledText();
  syncFacePointerText();
  syncTestFieldState();
  tabController.syncTabUi();
}

function render(payload) {
  latestSchema = normalizeSchema(payload?.schema?.mouse_companion || payload?.schema || {});
  latestState = normalizeState(payload?.state?.mouse_companion || payload?.state || {});
  latestRuntimeState = normalizeRuntimeState(payload?.state?.mouse_companion_runtime || {});
  const mount = mountIfNeeded();
  if (!mount) {
    return;
  }
  writeToDom(latestState, latestSchema);
  writeRuntimeToDom(latestRuntimeState);
}

function read() {
  const mount = mountIfNeeded();
  if (!mount) {
    return normalizeState(latestState);
  }
  latestState = readFromDom();
  return latestState;
}

function onAction(_action) {
  // Reserved for future interaction hooks.
}

window.MfxMouseCompanionSection = {
  render,
  read,
  onAction,
};
