import { isMacosPlatform } from './platform.js';

function asObject(value) {
  return value && typeof value === 'object' ? value : null;
}

function asText(value) {
  return `${value || ''}`.trim();
}

function asNumber(value, fallback = 0) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

export function normalizeGestureRouteStatus(state) {
  const source = asObject(state)?.input_automation_gesture_route_status;
  if (!asObject(source)) {
    return null;
  }
  const modifiers = asObject(source.last_modifiers) || {};
  return {
    automationEnabled: source.automation_enabled === true,
    gestureEnabled: source.gesture_enabled === true,
    buttonlessGestureEnabled: source.buttonless_gesture_enabled === true,
    pointerButtonDown: source.pointer_button_down === true,
    gestureMappingCount: asNumber(source.gesture_mapping_count, 0),
    buttonlessGestureMappingCount: asNumber(source.buttonless_gesture_mapping_count, 0),
    lastStage: asText(source.last_stage),
    lastReason: asText(source.last_reason),
    lastGestureId: asText(source.last_gesture_id),
    lastTriggerButton: asText(source.last_trigger_button),
    lastMatched: source.last_matched === true,
    lastInjected: source.last_injected === true,
    lastUsedCustom: source.last_used_custom === true,
    lastUsedPreset: source.last_used_preset === true,
    lastSamplePointCount: asNumber(source.last_sample_point_count, 0),
    modifiers: {
      primary: modifiers.primary === true,
      shift: modifiers.shift === true,
      alt: modifiers.alt === true,
    },
  };
}

function textByKey(texts, key, fallback) {
  const value = asText(asObject(texts)?.[key]);
  return value || fallback;
}

export function boolLabel(value, texts = {}) {
  return value
    ? textByKey(texts, 'yes', 'Yes')
    : textByKey(texts, 'no', 'No');
}

export function formatGestureRouteModifiers(modifiers, platform, texts = {}) {
  const source = asObject(modifiers) || {};
  const parts = [];
  if (source.primary) {
    parts.push(isMacosPlatform(platform) ? 'Cmd' : 'Ctrl');
  }
  if (source.shift) {
    parts.push('Shift');
  }
  if (source.alt) {
    parts.push(isMacosPlatform(platform) ? 'Option' : 'Alt');
  }
  if (parts.length > 0) {
    return parts.join('+');
  }
  return textByKey(texts, 'modifierEmpty', 'None');
}

export function gestureRouteSourceLabel(routeStatus, texts = {}) {
  if (routeStatus?.lastUsedCustom) {
    return textByKey(texts, 'sourceCustom', 'Custom');
  }
  if (routeStatus?.lastUsedPreset) {
    return textByKey(texts, 'sourcePreset', 'Preset');
  }
  return textByKey(texts, 'sourceUnknown', 'Unknown');
}
