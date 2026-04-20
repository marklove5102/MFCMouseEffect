import { isMacosPlatform, isWindowsPlatform, normalizeRuntimePlatform } from './platform.js';
import { normalizeTriggerChain, serializeTriggerChain } from './trigger-chain.js';

export const CAPTURE_TARGET_KEYS = 'keys';
export const CAPTURE_TARGET_MODIFIERS = 'modifiers';

function firstExecutableActionText(actions) {
  const source = Array.isArray(actions) ? actions : [];
  for (const action of source) {
    const type = `${action?.type || 'send_shortcut'}`.trim().toLowerCase();
    if (type === 'send_shortcut') {
      const shortcut = `${action?.shortcut || ''}`.trim();
      if (shortcut) {
        return shortcut;
      }
      continue;
    }
    if (type === 'open_url') {
      const url = `${action?.url || ''}`.trim();
      if (url) {
        return url;
      }
    }
    if (type === 'launch_app') {
      const appPath = `${action?.app_path ?? action?.appPath ?? ''}`.trim();
      if (appPath) {
        return appPath;
      }
    }
  }
  return '';
}

export function normalizedPlatform(platform) {
  return normalizeRuntimePlatform(platform);
}

export function scopeFileAccept(platform) {
  if (isMacosPlatform(platform)) {
    return '.app';
  }
  if (isWindowsPlatform(platform)) {
    return '.exe';
  }
  return '';
}

export function isModifierToken(token) {
  const text = `${token || ''}`.trim().toLowerCase();
  return text === 'ctrl'
    || text === 'shift'
    || text === 'alt'
    || text === 'option'
    || text === 'cmd'
    || text === 'command'
    || text === 'win'
    || text === 'meta'
    || text === 'os'
    || text === 'super';
}

export function isModifierOnlyShortcut(shortcut) {
  const tokens = `${shortcut || ''}`.split('+').map((item) => `${item || ''}`.trim()).filter((item) => !!item);
  if (tokens.length === 0) {
    return false;
  }
  return tokens.every((token) => isModifierToken(token));
}

function parseModifierFlagsFromShortcut(shortcutText) {
  const text = `${shortcutText || ''}`.trim();
  if (!text) {
    return { primary: false, shift: false, alt: false };
  }
  const tokens = text.split('+').map((item) => `${item || ''}`.trim().toLowerCase());
  let primary = false;
  let shift = false;
  let alt = false;
  for (const token of tokens) {
    if (!token) {
      continue;
    }
    if (token === 'ctrl'
      || token === 'control'
      || token === 'cmd'
      || token === 'command'
      || token === 'meta'
      || token === 'win'
      || token === 'windows'
      || token === 'os'
      || token === 'super') {
      primary = true;
    } else if (token === 'shift') {
      shift = true;
    } else if (token === 'alt' || token === 'option' || token === 'opt') {
      alt = true;
    }
  }
  return { primary, shift, alt };
}

export function buildModifierPayloadFromShortcut(shortcut, emptyMode = 'any') {
  const flags = parseModifierFlagsFromShortcut(shortcut);
  if (flags.primary || flags.shift || flags.alt) {
    return {
      mode: 'exact',
      primary: flags.primary,
      shift: flags.shift,
      alt: flags.alt,
    };
  }
  return {
    mode: emptyMode === 'none' ? 'none' : 'any',
    primary: false,
    shift: false,
    alt: false,
  };
}

export function normalizedModifiersForRow(row) {
  const source = row?.modifiers || {};
  const mode = `${source.mode || 'any'}`.trim().toLowerCase();
  return {
    mode: mode === 'none' || mode === 'exact' ? mode : 'any',
    primary: !!source.primary,
    shift: !!source.shift,
    alt: !!source.alt,
  };
}

export function modifierPrimaryLabel(texts, platform) {
  return texts.modifierPrimary || (isMacosPlatform(platform) ? 'Cmd' : 'Ctrl');
}

export function modifierAltLabel(texts, platform) {
  return texts.modifierAlt || (isMacosPlatform(platform) ? 'Option' : 'Alt');
}

export function modifierShortcutTextForRow(row, texts, platform) {
  const modifiers = normalizedModifiersForRow(row);
  if (modifiers.mode === 'any') {
    return texts.gestureTriggerModifiersAny || 'Any modifier';
  }
  if (modifiers.mode === 'none') {
    return texts.gestureTriggerModifiersNone || 'No modifier';
  }
  const parts = [];
  if (modifiers.primary) {
    parts.push(modifierPrimaryLabel(texts, platform));
  }
  if (modifiers.shift) {
    parts.push(texts.modifierShift || 'Shift');
  }
  if (modifiers.alt) {
    parts.push(modifierAltLabel(texts, platform));
  }
  return parts.length > 0
    ? parts.join('+')
    : (texts.gestureTriggerModifiersAny || 'Any modifier');
}

export function modifierInputValueForRow(row, texts, platform) {
  const modifiers = normalizedModifiersForRow(row);
  if (modifiers.mode !== 'exact') {
    return '';
  }
  return modifierShortcutTextForRow(row, texts, platform);
}

export function modifierInputPlaceholderForRow(row, texts, platform) {
  const modifiers = normalizedModifiersForRow(row);
  if (modifiers.mode === 'none') {
    return texts.gestureTriggerModifiersNonePlaceholder || 'Optional (empty means no modifier)';
  }
  if (modifiers.mode === 'any') {
    return texts.gestureTriggerModifiersAny || 'Any modifier';
  }
  return texts.gestureTriggerShortcutPlaceholder || 'Cmd / Ctrl+Shift';
}

export function chainForRow(row, options) {
  return normalizeTriggerChain(row?.triggerChain || row?.trigger || '', options, options[0]?.value || '');
}

export function triggerValueFromEvent(event, fallbackTrigger, options) {
  const detail = event?.detail || {};
  const fallback = `${fallbackTrigger || options[0]?.value || ''}`;

  if (detail.value !== undefined && detail.value !== null) {
    return serializeTriggerChain(detail.value, options, fallback);
  }
  if (detail.chain !== undefined && detail.chain !== null) {
    return serializeTriggerChain(detail.chain, options, fallback);
  }

  if (event?.target && event.target.value !== undefined) {
    return serializeTriggerChain(event.target.value, options, fallback);
  }

  return '';
}

export function triggerLabel(value, options) {
  const normalized = `${value || ''}`.trim();
  if (!normalized) {
    return '';
  }
  for (const option of options || []) {
    if (`${option?.value || ''}`.trim() === normalized) {
      return `${option?.label || normalized}`.trim() || normalized;
    }
  }
  return normalized;
}

export function triggerSummaryForRow(row, options) {
  const labels = chainForRow(row, options)
    .map((value) => triggerLabel(value, options))
    .filter((value) => !!value);
  if (labels.length === 0) {
    return '-';
  }
  return labels.join(' -> ');
}

export function gestureButtonForRow(row, gestureButtonOptions) {
  const fallback = `${gestureButtonOptions?.[0]?.value || 'right'}`.trim().toLowerCase();
  const value = `${row?.triggerButton || fallback}`.trim().toLowerCase();
  if (value === 'left' || value === 'middle' || value === 'right' || value === 'none') {
    return value;
  }
  if (fallback === 'left' || fallback === 'middle' || fallback === 'right' || fallback === 'none') {
    return fallback;
  }
  return 'right';
}

export function gestureButtonLabel(value, gestureButtonOptions) {
  const normalized = `${value || ''}`.trim();
  if (!normalized) {
    return '';
  }
  for (const option of gestureButtonOptions || []) {
    if (`${option?.value || ''}`.trim() === normalized) {
      return `${option?.label || normalized}`.trim() || normalized;
    }
  }
  return normalized;
}

export function gestureSummaryForRow(row, { kind, options, texts, gestureButtonOptions }) {
  if (kind !== 'gesture') {
    return triggerSummaryForRow(row, options);
  }
  const mode = `${row?.gesturePattern?.mode || 'preset'}`.trim().toLowerCase();
  if (mode === 'custom') {
    const thresholdRaw = row?.gesturePattern?.matchThresholdPercent !== undefined
      ? row.gesturePattern.matchThresholdPercent
      : row?.gesturePattern?.match_threshold_percent;
    const threshold = Number(thresholdRaw);
    const percent = Number.isFinite(threshold)
      ? Math.max(50, Math.min(95, Math.round(threshold)))
      : 75;
    return `${gestureButtonLabel(gestureButtonForRow(row, gestureButtonOptions), gestureButtonOptions)} · ${texts.gestureModeCustom || 'Custom Draw'} ${percent}%`;
  }
  return `${gestureButtonLabel(gestureButtonForRow(row, gestureButtonOptions), gestureButtonOptions)} · ${triggerSummaryForRow(row, options)}`;
}

export function scopeModeForRow(row) {
  const mode = `${row?.appScopeMode || row?.appScopeType || 'all'}`.trim().toLowerCase();
  return mode === 'selected' || mode === 'process' ? 'selected' : 'all';
}

export function scopeAppsForRow(row) {
  return Array.isArray(row?.appScopeApps) ? row.appScopeApps : [];
}

export function scopeDraftForRow(row) {
  return `${row?.appScopeDraft || ''}`;
}

export function scopeSummaryForRow(row, texts) {
  if (scopeModeForRow(row) === 'all') {
    return texts.scopeAllLabel || 'All Apps';
  }
  const apps = scopeAppsForRow(row);
  if (apps.length === 0) {
    return texts.scopeSelectedEmpty || 'No app selected';
  }
  if (apps.length === 1) {
    return apps[0];
  }
  return `${apps[0]} +${apps.length - 1}`;
}

export function shortcutSummaryForRow(row, texts) {
  const actionText = firstExecutableActionText(row?.actions);
  if (actionText) {
    return actionText;
  }
  return texts.actionEmpty || texts.shortcutEmpty || 'No action';
}

export function catalogMetaText(entry) {
  const exe = `${entry?.exe || ''}`.trim();
  const source = `${entry?.source || ''}`.trim().toLowerCase();
  if (!source) {
    return exe;
  }
  return `${exe} (${source})`;
}
