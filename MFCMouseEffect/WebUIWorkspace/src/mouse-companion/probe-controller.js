export const MOUSE_COMPANION_DEFAULT_PROBE_INPUT = {
  x: 640,
  y: 360,
  button: 1,
  delta: 120,
  hold_ms: 420,
};

export const MOUSE_COMPANION_DEFAULT_PROBE_RESULT = {
  text: '-',
  tone: '',
};

function sleep(ms) {
  return new Promise((resolve) => {
    window.setTimeout(resolve, ms);
  });
}

function probeApiUnavailable(error) {
  const text = `${error?.message || error || ''}`.trim().toLowerCase();
  return text.includes('not found');
}

export function createMouseCompanionProbeController(options) {
  const opts = options && typeof options === 'object' ? options : {};
  const byId = typeof opts.byId === 'function' ? opts.byId : () => null;
  const clampInt = typeof opts.clampInt === 'function' ? opts.clampInt : (value) => Number(value) || 0;
  const resolveApiPost =
    typeof opts.resolveApiPost === 'function' ? opts.resolveApiPost : () => null;
  const normalizeRuntimeState =
    typeof opts.normalizeRuntimeState === 'function'
      ? opts.normalizeRuntimeState
      : (value) => value || {};
  const normalizeProbeRuntimeResponse =
    typeof opts.normalizeProbeRuntimeResponse === 'function'
      ? opts.normalizeProbeRuntimeResponse
      : (value) => value || {};
  const onRuntimeState =
    typeof opts.onRuntimeState === 'function' ? opts.onRuntimeState : () => {};
  const getRuntimeState =
    typeof opts.getRuntimeState === 'function' ? opts.getRuntimeState : () => ({});

  let latestProbeInput = { ...MOUSE_COMPANION_DEFAULT_PROBE_INPUT };
  let latestProbeResult = { ...MOUSE_COMPANION_DEFAULT_PROBE_RESULT };

  function setProbeResult(text, tone) {
    latestProbeResult = {
      text: `${text || '-'}`,
      tone: `${tone || ''}`,
    };
    const node = byId('mc_probe_result');
    if (!node) {
      return;
    }
    node.textContent = latestProbeResult.text;
    node.classList.toggle('is-ok', latestProbeResult.tone === 'ok');
    node.classList.toggle('is-warn', latestProbeResult.tone === 'warn');
  }

  function readProbeInput() {
    const x = clampInt(byId('mc_probe_x')?.value, -20000, 20000, latestProbeInput.x);
    const y = clampInt(byId('mc_probe_y')?.value, -20000, 20000, latestProbeInput.y);
    const button = clampInt(byId('mc_probe_button')?.value, 1, 3, latestProbeInput.button);
    const delta = clampInt(byId('mc_probe_delta')?.value, -4000, 4000, latestProbeInput.delta);
    const holdMs = clampInt(byId('mc_probe_hold_ms')?.value, 0, 60000, latestProbeInput.hold_ms);
    latestProbeInput = { x, y, button, delta, hold_ms: holdMs };
    return latestProbeInput;
  }

  function writeProbeInput() {
    const probeX = byId('mc_probe_x');
    if (probeX) {
      probeX.value = `${latestProbeInput.x}`;
    }
    const probeY = byId('mc_probe_y');
    if (probeY) {
      probeY.value = `${latestProbeInput.y}`;
    }
    const probeButton = byId('mc_probe_button');
    if (probeButton) {
      probeButton.value = `${latestProbeInput.button}`;
    }
    const probeDelta = byId('mc_probe_delta');
    if (probeDelta) {
      probeDelta.value = `${latestProbeInput.delta}`;
    }
    const probeHoldMs = byId('mc_probe_hold_ms');
    if (probeHoldMs) {
      probeHoldMs.value = `${latestProbeInput.hold_ms}`;
    }
  }

  function restoreProbeResult() {
    setProbeResult(latestProbeResult.text, latestProbeResult.tone);
  }

  async function dispatchProbeEvent(eventName, options = {}) {
    const localOpts = options && typeof options === 'object' ? options : {};
    const reportResult = localOpts.reportResult !== false;
    const apiPost = resolveApiPost();
    if (typeof apiPost !== 'function') {
      setProbeResult('probe api unavailable in current page runtime', 'warn');
      return null;
    }

    const probe = readProbeInput();
    try {
      const response = await apiPost('/api/mouse-companion/test-dispatch', {
        event: eventName,
        x: probe.x,
        y: probe.y,
        button: probe.button,
        delta: probe.delta,
        hold_ms: probe.hold_ms,
      });
      if (!response || response.ok !== true) {
        setProbeResult(`probe failed: ${response?.error || 'unknown_error'}`, 'warn');
        return null;
      }

      const runtimeState = normalizeRuntimeState(normalizeProbeRuntimeResponse(response));
      onRuntimeState(runtimeState);
      if (reportResult) {
        const actionName = `${response?.runtime?.last_action_name || runtimeState.last_action_name || '-'}`;
        setProbeResult(`ok: ${eventName} -> ${actionName}`, 'ok');
      }
      return response;
    } catch (error) {
      if (probeApiUnavailable(error)) {
        setProbeResult('test api disabled (set MFX_ENABLE_MOUSE_COMPANION_TEST_API=1)', 'warn');
      } else {
        const message = `${error?.message || error || 'unknown_error'}`.trim();
        setProbeResult(`probe error: ${message || 'unknown_error'}`, 'warn');
      }
      return null;
    }
  }

  async function runProbeSequence() {
    setProbeResult('running action sequence...', '');
    const sequence = [
      'status',
      'move',
      'scroll',
      'button_down',
      'button_up',
      'click',
      'hover_start',
      'hover_end',
      'hold_start',
      'hold_update',
      'hold_end',
    ];
    for (const eventName of sequence) {
      const response = await dispatchProbeEvent(eventName, { reportResult: false });
      if (!response) {
        return;
      }
      await sleep(36);
    }
    const actionName = `${getRuntimeState()?.last_action_name || '-'}`;
    setProbeResult(`sequence ok -> ${actionName}`, 'ok');
  }

  function bindProbeActions() {
    const bindProbeAction = (id, action) => {
      const node = byId(id);
      if (!node || typeof action !== 'function') {
        return;
      }
      node.addEventListener('click', () => {
        void action();
      });
    };
    bindProbeAction('mc_probe_status', () => dispatchProbeEvent('status'));
    bindProbeAction('mc_probe_move', () => dispatchProbeEvent('move'));
    bindProbeAction('mc_probe_scroll', () => dispatchProbeEvent('scroll'));
    bindProbeAction('mc_probe_hover_start', () => dispatchProbeEvent('hover_start'));
    bindProbeAction('mc_probe_hover_end', () => dispatchProbeEvent('hover_end'));
    bindProbeAction('mc_probe_hold_start', () => dispatchProbeEvent('hold_start'));
    bindProbeAction('mc_probe_hold_update', () => dispatchProbeEvent('hold_update'));
    bindProbeAction('mc_probe_hold_end', () => dispatchProbeEvent('hold_end'));
    bindProbeAction('mc_probe_button_down', () => dispatchProbeEvent('button_down'));
    bindProbeAction('mc_probe_button_up', () => dispatchProbeEvent('button_up'));
    bindProbeAction('mc_probe_click', () => dispatchProbeEvent('click'));
    bindProbeAction('mc_probe_sequence', runProbeSequence);
  }

  return {
    writeProbeInput,
    restoreProbeResult,
    bindProbeActions,
  };
}
