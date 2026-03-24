import CursorDecorationFields from '../cursor-decoration/CursorDecorationFields.svelte';
import { createLazyMountBridge } from './lazy-mount.js';

let currentState = {
  enabled: false,
  plugin_id: 'ring',
  color_hex: '#ff5a5a',
  size_px: 22,
  alpha_percent: 82,
};

function toNumber(value, fallback) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function normalizeDecoration(input) {
  const value = input || {};
  return {
    enabled: value.enabled === true,
    plugin_id: `${value.plugin_id || 'ring'}`.trim() || 'ring',
    color_hex: `${value.color_hex || '#ff5a5a'}`.trim() || '#ff5a5a',
    size_px: toNumber(value.size_px, 22),
    alpha_percent: toNumber(value.alpha_percent, 82),
  };
}

const bridge = createLazyMountBridge({
  mountId: 'cursor_decoration_settings_mount',
  initialProps: {
    pluginOptions: [],
    decoration: currentState,
  },
  createComponent: (mountNode, props) => {
    const instance = new CursorDecorationFields({
      target: mountNode,
      props,
    });
    instance.$on('change', (event) => {
      currentState = normalizeDecoration(event?.detail || {});
    });
    return instance;
  },
});

function render(payload) {
  const schema = payload?.schema || {};
  const decoration = normalizeDecoration(payload?.decoration || {});
  currentState = decoration;
  bridge.updateProps({
    pluginOptions: schema.cursor_decoration?.plugins || [],
    decoration,
  });
}

function read() {
  return normalizeDecoration(currentState);
}

window.MfxCursorDecorationSection = {
  render,
  read,
};
