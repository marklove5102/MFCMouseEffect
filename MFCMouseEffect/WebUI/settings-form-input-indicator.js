(() => {
  let indicatorBound = false;

  function el(id) {
    return document.getElementById(id);
  }

  function inputIndicatorSection() {
    return window.MfxInputIndicatorSection || null;
  }

  function syncIndicatorPositionUi() {
    const section = inputIndicatorSection();
    if (section && typeof section.syncIndicatorPositionUi === 'function') {
      section.syncIndicatorPositionUi();
      return;
    }

    const mode = (el('ii_position_mode')?.value || 'relative').toString();
    const relativeRow = el('ii_offset_x')?.closest('.pair');
    const absoluteRow = el('ii_absolute_x')?.closest('.pair');
    if (relativeRow) {
      relativeRow.style.opacity = (mode === 'relative') ? '1' : '0.45';
    }
    if (absoluteRow) {
      absoluteRow.style.opacity = (mode === 'absolute') ? '1' : '0.45';
    }

    const targetMon = (el('ii_target_monitor')?.value || 'cursor').toString();
    const perMonitorContainer = el('ii_per_monitor_overrides');
    if (perMonitorContainer) {
      perMonitorContainer.style.display = (mode === 'absolute' && targetMon === 'custom') ? 'block' : 'none';
    }
  }

  function bindIndicatorEvents() {
    if (indicatorBound) {
      return;
    }
    const section = inputIndicatorSection();
    if (section) {
      indicatorBound = true;
      return;
    }
    indicatorBound = true;

    const positionMode = el('ii_position_mode');
    if (positionMode) {
      positionMode.addEventListener('change', syncIndicatorPositionUi);
    }
    const targetMonitor = el('ii_target_monitor');
    if (targetMonitor) {
      targetMonitor.addEventListener('change', syncIndicatorPositionUi);
    }
  }

  function buildPerMonitorUI(monitors, overrides, texts) {
    const container = el('ii_per_monitor_overrides');
    if (!container) {
      return;
    }
    container.innerHTML = '';

    if (!Array.isArray(monitors) || monitors.length === 0) {
      const tip = document.createElement('div');
      tip.className = 'pm-empty';
      tip.textContent = texts.pm_no_monitors || 'No monitors detected.';
      container.appendChild(tip);
      return;
    }

    const monLabel = texts.pm_monitor || 'Monitor';
    const primaryBadge = texts.pm_primary_badge || 'Primary';
    for (let idx = 0; idx < monitors.length; idx += 1) {
      const monitor = monitors[idx];
      const monitorId = monitor.id;
      const width = (monitor.right || 0) - (monitor.left || 0);
      const height = (monitor.bottom || 0) - (monitor.top || 0);
      const resolution = (width > 0 && height > 0) ? ` (${width}x${height})` : '';
      const badge = monitor.is_primary ? ` * ${primaryBadge}` : '';
      const labelText = `${monLabel} ${idx + 1}${resolution}${badge}`;

      const override = overrides ? overrides[monitorId] : null;
      const enabled = !!(override && override.enabled);
      const valueX = (override && override.absolute_x !== undefined) ? override.absolute_x : 40;
      const valueY = (override && override.absolute_y !== undefined) ? override.absolute_y : 40;

      const row = document.createElement('div');
      row.className = 'monitor-row';
      if (!enabled) {
        row.classList.add('is-disabled');
      }

      const checkbox = document.createElement('input');
      checkbox.type = 'checkbox';
      checkbox.id = `ii_ov_${monitorId}_en`;
      checkbox.className = 'monitor-toggle';
      checkbox.checked = enabled;

      const label = document.createElement('label');
      label.htmlFor = checkbox.id;
      label.className = 'monitor-name';
      label.title = labelText;
      label.textContent = labelText;

      const inputX = document.createElement('input');
      inputX.type = 'number';
      inputX.id = `ii_ov_${monitorId}_x`;
      inputX.className = 'monitor-input';
      inputX.value = valueX;
      inputX.disabled = !enabled;
      inputX.title = 'X';
      inputX.placeholder = 'X';

      const inputY = document.createElement('input');
      inputY.type = 'number';
      inputY.id = `ii_ov_${monitorId}_y`;
      inputY.className = 'monitor-input';
      inputY.value = valueY;
      inputY.disabled = !enabled;
      inputY.title = 'Y';
      inputY.placeholder = 'Y';

      checkbox.addEventListener('change', () => {
        const checked = checkbox.checked;
        inputX.disabled = !checked;
        inputY.disabled = !checked;
        row.classList.toggle('is-disabled', !checked);
      });

      row.appendChild(checkbox);
      row.appendChild(label);
      row.appendChild(inputX);
      row.appendChild(inputY);
      container.appendChild(row);
    }
  }

  function readPerMonitorUI(monitors) {
    const result = {};
    if (!Array.isArray(monitors)) {
      return result;
    }
    for (const monitor of monitors) {
      const monitorId = monitor.id;
      const checkbox = el(`ii_ov_${monitorId}_en`);
      const inputX = el(`ii_ov_${monitorId}_x`);
      const inputY = el(`ii_ov_${monitorId}_y`);
      if (!checkbox || !inputX || !inputY) {
        continue;
      }
      result[monitorId] = {
        enabled: checkbox.checked,
        absolute_x: Number(inputX.value),
        absolute_y: Number(inputY.value),
      };
    }
    return result;
  }

  function renderInputIndicator(schema, appState, texts, wasmAction, helpers) {
    const indicator = appState.input_indicator || appState.mouse_indicator || {};
    const indicatorRouteStatus = appState.input_indicator_wasm_route_status || {};
    const section = inputIndicatorSection();
    if (section && typeof section.render === 'function') {
      section.render({
        schema,
        indicator,
        wasmState: {
          ...(appState.wasm || {}),
          input_indicator_wasm_route_status: indicatorRouteStatus,
        },
        onWasmAction: wasmAction,
        texts,
      });
      return;
    }

    helpers.fillSelect(
      'ii_position_mode',
      schema.input_indicator_position_modes,
      indicator.position_mode || 'relative');
    helpers.fillSelect(
      'ii_render_mode',
      schema.input_indicator_render_modes,
      indicator.render_mode || 'native');
    helpers.fillSelect('ii_target_monitor', schema.target_monitor_options, indicator.target_monitor || 'cursor');
    helpers.fillSelect('ii_key_display_mode', schema.key_display_modes, indicator.key_display_mode || 'all');
    helpers.fillSelect(
      'ii_key_label_layout_mode',
      schema.key_label_layout_modes,
      indicator.key_label_layout_mode || 'fixed_font');

    buildPerMonitorUI(schema.monitors, indicator.per_monitor_overrides, texts);

    helpers.setChecked('ii_enabled', indicator.enabled !== false);
    helpers.setChecked('ii_keyboard_enabled', indicator.keyboard_enabled !== false);
    helpers.setChecked('ii_wasm_fallback', indicator.wasm_fallback_to_native !== false);
    helpers.setNum('ii_offset_x', indicator.offset_x);
    helpers.setNum('ii_offset_y', indicator.offset_y);
    helpers.setNum('ii_absolute_x', indicator.absolute_x);
    helpers.setNum('ii_absolute_y', indicator.absolute_y);
    helpers.setNum('ii_size_px', indicator.size_px);
    helpers.setNum('ii_duration_ms', indicator.duration_ms);

    syncIndicatorPositionUi();
  }

  function readInputIndicatorState(schema, helpers) {
    const section = inputIndicatorSection();
    if (section && typeof section.read === 'function') {
      return section.read();
    }
    return {
      enabled: helpers.getChecked('ii_enabled'),
      keyboard_enabled: helpers.getChecked('ii_keyboard_enabled'),
      render_mode: helpers.getText('ii_render_mode') || 'native',
      wasm_fallback_to_native: helpers.getChecked('ii_wasm_fallback'),
      wasm_manifest_path: '',
      position_mode: helpers.getText('ii_position_mode') || 'relative',
      offset_x: helpers.getNum('ii_offset_x'),
      offset_y: helpers.getNum('ii_offset_y'),
      absolute_x: helpers.getNum('ii_absolute_x'),
      absolute_y: helpers.getNum('ii_absolute_y'),
      target_monitor: helpers.getText('ii_target_monitor') || 'cursor',
      key_display_mode: helpers.getText('ii_key_display_mode') || 'all',
      key_label_layout_mode: helpers.getText('ii_key_label_layout_mode') || 'fixed_font',
      per_monitor_overrides: readPerMonitorUI(schema.monitors),
      size_px: helpers.getNum('ii_size_px'),
      duration_ms: helpers.getNum('ii_duration_ms'),
    };
  }

  window.MfxSettingsInputIndicator = {
    bindIndicatorEvents,
    readInputIndicatorState,
    renderInputIndicator,
    syncIndicatorPositionUi,
  };
})();
