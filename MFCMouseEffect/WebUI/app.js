(() => {
  const el = (id) => document.getElementById(id);
  const core = window.MfxAppCore;
  if (!core) {
    throw new Error('app-core unavailable');
  }

  const gestureDebug = window.MfxGestureDebug ? window.MfxGestureDebug.init(core) : null;
  core.setGestureDebug(gestureDebug);

  const actions = window.MfxAppActions ? window.MfxAppActions.init(core) : null;
  if (actions) {
    core.setActionHandlers({
      general: actions.handleGeneralAction,
      wasm: actions.handleWasmAction,
    });
  }

  function handleLanguageChange() {
    const langEl = el('ui_language');
    if (!langEl) return;
    try {
      core.applyI18n(langEl.value);
    } catch (_error) {
      // Ignore i18n runtime errors on language switch to keep UI usable.
    }
    if (core.getConnectionState() !== 'unknown') {
      core.markConnection(core.getConnectionState(), true);
    }
  }

  function bindLegacyActionButtons() {
    if (!actions) return;
    const reloadBtn = el('btnReload');
    if (reloadBtn) reloadBtn.addEventListener('click', () => { actions.handleReloadAction(); });

    const saveBtn = el('btnSave');
    if (saveBtn) saveBtn.addEventListener('click', () => { actions.handleSaveAction(); });

    const resetBtn = el('btnReset');
    if (resetBtn) resetBtn.addEventListener('click', () => { actions.handleResetAction(); });

    const stopBtn = el('btnStop');
    if (stopBtn) stopBtn.addEventListener('click', () => { actions.handleStopAction(); });
  }

  function bindShellActions() {
    if (!actions) return;
    const shell = window.MfxWebShell || null;
    if (!shell || typeof shell.onAction !== 'function') {
      bindLegacyActionButtons();
      return;
    }
    shell.onAction((type) => {
      if (type === 'reload') {
        actions.handleReloadAction();
        return;
      }
      if (type === 'save') {
        actions.handleSaveAction();
        return;
      }
      if (type === 'reset') {
        actions.handleResetAction();
        return;
      }
      if (type === 'stop') {
        actions.handleStopAction();
      }
    });
  }

  function bindLanguageChange() {
    const langEl = el('ui_language');
    if (!langEl) return;
    langEl.addEventListener('change', handleLanguageChange);
  }

  bindShellActions();
  bindLanguageChange();

  window.addEventListener('hashchange', () => {
    if (gestureDebug && typeof gestureDebug.refresh === 'function') {
      gestureDebug.refresh();
    }
  });

  if (window.MfxSectionWorkspace && typeof window.MfxSectionWorkspace.init === 'function') {
    window.MfxSectionWorkspace.init();
  }

  core.startHealthCheck();
  core.reload().then(() => {
    if (window.MfxSectionWorkspace && typeof window.MfxSectionWorkspace.refresh === 'function') {
      window.MfxSectionWorkspace.refresh();
    }
  }).catch(e => {
    core.handleReloadFailure(e);
  });
})();
