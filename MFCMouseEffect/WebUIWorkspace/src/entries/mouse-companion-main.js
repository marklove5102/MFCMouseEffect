import {
  MOUSE_COMPANION_CHECKBOX_FIELDS,
  MOUSE_COMPANION_DEFAULT_ACTIVE_TAB,
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
  test_press_lift_px_range: { min: 0, max: 320, step: 1 },
  test_smoothing_percent_range: { min: 0, max: 95, step: 1 },
};

const DEFAULT_STATE = {
  enabled: false,
  model_path: 'MFCMouseEffect/Assets/Pet3D/source/pet-main.glb',
  action_library_path: 'MFCMouseEffect/Assets/Pet3D/source/pet-actions.json',
  appearance_profile_path: 'MFCMouseEffect/Assets/Pet3D/source/pet-appearance.json',
  edge_clamp_mode: 'soft',
  size_px: 112,
  offset_x: 18,
  offset_y: 26,
  press_lift_px: 24,
  smoothing_percent: 68,
  follow_threshold_px: 2,
  release_hold_ms: 120,
  use_test_profile: false,
  test_press_lift_px: 48,
  test_smoothing_percent: 32,
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

function normalizeRange(value, fallback) {
  const source = value && typeof value === 'object' ? value : {};
  return {
    min: clampInt(source.min, fallback.min, fallback.max, fallback.min),
    max: clampInt(source.max, fallback.min, fallback.max, fallback.max),
    step: clampInt(source.step, 1, 1000, fallback.step),
  };
}

function normalizeSchema(value) {
  const source = value && typeof value === 'object' ? value : {};
  const normalizeModeOptions = (input) => {
    if (!Array.isArray(input)) {
      return [...DEFAULT_SCHEMA.edge_clamp_modes];
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
      return [...DEFAULT_SCHEMA.edge_clamp_modes];
    }
    return normalized;
  };
  return {
    edge_clamp_modes: normalizeModeOptions(source.edge_clamp_modes),
    size_px_range: normalizeRange(source.size_px_range, DEFAULT_SCHEMA.size_px_range),
    offset_range: normalizeRange(source.offset_range, DEFAULT_SCHEMA.offset_range),
    press_lift_px_range: normalizeRange(source.press_lift_px_range, DEFAULT_SCHEMA.press_lift_px_range),
    smoothing_percent_range: normalizeRange(source.smoothing_percent_range, DEFAULT_SCHEMA.smoothing_percent_range),
    follow_threshold_px_range: normalizeRange(source.follow_threshold_px_range, DEFAULT_SCHEMA.follow_threshold_px_range),
    release_hold_ms_range: normalizeRange(source.release_hold_ms_range, DEFAULT_SCHEMA.release_hold_ms_range),
    test_press_lift_px_range: normalizeRange(source.test_press_lift_px_range, DEFAULT_SCHEMA.test_press_lift_px_range),
    test_smoothing_percent_range: normalizeRange(source.test_smoothing_percent_range, DEFAULT_SCHEMA.test_smoothing_percent_range),
  };
}

function normalizeState(value) {
  const source = value && typeof value === 'object' ? value : {};
  const modelPath = `${source.model_path || ''}`.trim();
  const actionLibraryPath = `${source.action_library_path || ''}`.trim();
  const appearanceProfilePath = `${source.appearance_profile_path || ''}`.trim();
  const edgeClampMode = `${source.edge_clamp_mode ?? ''}`.trim().toLowerCase();
  return {
    enabled: !!source.enabled,
    model_path: modelPath || DEFAULT_STATE.model_path,
    action_library_path: actionLibraryPath || DEFAULT_STATE.action_library_path,
    appearance_profile_path: appearanceProfilePath || DEFAULT_STATE.appearance_profile_path,
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
    use_test_profile: !!source.use_test_profile,
    test_press_lift_px: clampInt(source.test_press_lift_px, 0, 320, DEFAULT_STATE.test_press_lift_px),
    test_smoothing_percent: clampInt(source.test_smoothing_percent, 0, 95, DEFAULT_STATE.test_smoothing_percent),
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

function applySelectOptions(selectId, options, selected) {
  const node = byId(selectId);
  if (!node) {
    return;
  }
  const normalized = Array.isArray(options) && options.length > 0
    ? options
    : DEFAULT_SCHEMA.edge_clamp_modes;
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
  node.value = selectedValue || DEFAULT_STATE.edge_clamp_mode;
}

function syncTestFieldState() {
  const useTestProfile = readChecked('mc_use_test_profile');
  const testPressLift = byId('mc_test_press_lift_px');
  const testSmoothing = byId('mc_test_smoothing_percent');
  if (testPressLift) {
    testPressLift.disabled = !useTestProfile;
  }
  if (testSmoothing) {
    testSmoothing.disabled = !useTestProfile;
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
  applySelectOptions('mc_edge_clamp_mode', schema.edge_clamp_modes, state.edge_clamp_mode);

  for (const binding of MOUSE_COMPANION_RANGE_BINDINGS) {
    applyRange(binding.id, schema[binding.schemaKey]);
  }
  probeController.writeProbeInput();
  probeController.restoreProbeResult();

  syncEnabledText();
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
