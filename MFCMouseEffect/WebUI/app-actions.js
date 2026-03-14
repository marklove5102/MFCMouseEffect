(() => {
  function init(appCore) {
    const core = appCore;
    if (!core) {
      throw new Error('app-core unavailable');
    }

    async function handleWasmAction(action, payload) {
      try {
        if (core.blockActionWhenDisconnected()) {
          return { ok: false, error: 'blocked' };
        }
        const body = payload || {};
        if (action === 'catalog') {
          return await core.apiPost('/api/wasm/catalog', body);
        }
        if (action === 'enable') {
          core.setStatus(core.statusText('status_wasm_enabling', 'Enabling WASM plugin...'));
          const result = await core.apiPost('/api/wasm/enable', {});
          await core.refreshAfterWasmAction();
          return result;
        }
        if (action === 'disable') {
          core.setStatus(core.statusText('status_wasm_disabling', 'Disabling WASM plugin...'));
          const result = await core.apiPost('/api/wasm/disable', {});
          await core.refreshAfterWasmAction();
          return result;
        }
        if (action === 'reload') {
          core.setStatus(core.statusText('status_wasm_reloading', 'Reloading WASM plugin...'));
          const result = await core.apiPost('/api/wasm/reload', {});
          await core.refreshAfterWasmAction();
          return result;
        }
        if (action === 'loadManifest') {
          core.setStatus(core.statusText('status_wasm_loading_manifest', 'Loading WASM manifest...'));
          const result = await core.apiPost('/api/wasm/load-manifest', body);
          await core.refreshAfterWasmAction();
          return result;
        }
        if (action === 'importSelected') {
          core.setStatus(core.statusText('status_wasm_importing', 'Importing WASM plugin...'));
          const result = await core.apiPost('/api/wasm/import-selected', body);
          await core.refreshAfterWasmAction();
          return result;
        }
        if (action === 'importFromFolderDialog') {
          core.setStatus(core.statusText('status_wasm_importing_folder', 'Importing WASM plugin folder...'));
          const result = await core.apiPost('/api/wasm/import-from-folder-dialog', body);
          await core.refreshAfterWasmAction();
          return result;
        }
        if (action === 'exportAll') {
          core.setStatus(core.statusText('status_wasm_exporting', 'Exporting WASM plugins...'));
          const result = await core.apiPost('/api/wasm/export-all', {});
          await core.refreshAfterWasmAction();
          return result;
        }
        if (action === 'setPolicy') {
          core.setStatus(core.statusText('status_wasm_updating_policy', 'Updating WASM policy...'));
          const result = await core.apiPost('/api/wasm/policy', body);
          await core.refreshAfterWasmAction();
          return result;
        }
        return { ok: false, error: 'unsupported action' };
      } catch (e) {
        if (e && e.code === 'unauthorized') {
          return { ok: false, error: 'unauthorized' };
        }
        core.setStatus(core.statusError('status_wasm_action_failed', 'WASM action failed: ', e), 'warn');
        return {
          ok: false,
          error: (e && e.message) ? e.message : String(e || ''),
        };
      }
    }

    async function handleGeneralAction(action, payload) {
      try {
        if (core.blockActionWhenDisconnected()) {
          return { ok: false, error: 'blocked' };
        }
        const body = payload || {};
        if (action === 'pickThemeCatalogRootPath') {
          core.setStatus(core.statusText('status_theme_catalog_picker_opening', 'Opening theme folder picker...'));
          const result = await core.apiPost('/api/theme/catalog-folder-dialog', body);
          if (result && !result.ok && result.error) {
            core.setStatus(core.statusText('status_theme_catalog_picker_failed', 'Theme folder picker failed: ') + result.error, 'warn');
          }
          return result;
        }
        return { ok: false, error: 'unsupported action' };
      } catch (e) {
        if (e && e.code === 'unauthorized') {
          return { ok: false, error: 'unauthorized' };
        }
        core.setStatus(core.statusError('status_theme_catalog_picker_failed', 'Theme folder picker failed: ', e), 'warn');
        return {
          ok: false,
          error: (e && e.message) ? e.message : String(e || ''),
        };
      }
    }

    function buildState() {
      const automation = (window.MfxAutomationUi && typeof window.MfxAutomationUi.read === 'function')
        ? window.MfxAutomationUi.read()
        : {
          enabled: false,
          mouse_mappings: [],
          gesture: {
            enabled: false,
            trigger_button: 'right',
            min_stroke_distance_px: 80,
            sample_step_px: 10,
            max_directions: 4,
            mappings: [],
          },
        };
      const form = core.settingsForm();
      const baseState = (form && typeof form.read === 'function')
        ? form.read()
        : {};

      return {
        ...baseState,
        automation,
      };
    }

    async function handleReloadAction() {
      try {
        if (core.blockActionWhenDisconnected()) return;
        core.setStatus(core.statusText('status_reloading', 'Reloading config...'));
        await core.apiPost('/api/reload', {});
        await core.reload();
      } catch (e) {
        if (e && e.code === 'unauthorized') return;
        core.setStatus(core.statusError('status_reload_failed', 'Reload failed: ', e), 'warn');
      }
    }

    async function handleSaveAction() {
      try {
        if (core.blockActionWhenDisconnected()) return;
        const automationValidation = (window.MfxAutomationUi && typeof window.MfxAutomationUi.validate === 'function')
          ? window.MfxAutomationUi.validate()
          : { ok: true };
        if (!automationValidation.ok) {
          core.setStatus(automationValidation.message || 'Please resolve mapping validation errors before applying.', 'warn');
          return;
        }
        core.setStatus(core.statusText('status_applying', 'Applying...'));
        const st = buildState();
        const previousThemeCatalogRootPath = `${core.getCachedState()?.theme_catalog_root_path || ''}`.trim();
        const requestedThemeCatalogRootPath = `${st?.theme_catalog_root_path || ''}`.trim();
        const shouldRefreshSchema = requestedThemeCatalogRootPath !== previousThemeCatalogRootPath;
        const res = await core.apiPost('/api/state', st);
        if (res.ok) {
          let latest = await core.apiGet('/api/state');
          const indicatorSync = await core.ensureInputIndicatorRuntimeManifestSynced(latest);
          latest = indicatorSync.state;
          if (shouldRefreshSchema) {
            const schema = await core.apiGet('/api/schema');
            core.renderSettingsSnapshot(schema, latest);
          } else {
            core.renderSettingsSnapshot(core.getCachedSchema(), latest);
          }
          if (indicatorSync.attempted && !indicatorSync.ok) {
            const suffix = indicatorSync.error || indicatorSync.errorCode || 'unknown error';
            core.setStatus(`Applied, but indicator plugin reload failed: ${suffix}`, 'warn');
            return;
          }
          const runtimeNotice = core.pickRuntimeNotice(latest);
          if (runtimeNotice) {
            core.setStatus(runtimeNotice.message, runtimeNotice.level === 'warn' ? 'warn' : 'ok');
          } else {
            core.setStatus(core.statusText('status_applied', 'Applied.'), 'ok');
          }
        } else {
          core.setStatus('Failed: ' + (res.error || ''), 'warn');
        }
      } catch (e) {
        if (e && e.code === 'unauthorized') return;
        core.setStatus(core.statusError('status_save_failed', 'Save failed: ', e), 'warn');
      }
    }

    async function handleResetAction() {
      try {
        if (core.blockActionWhenDisconnected()) return;
        const t = core.currentText();
        let confirmed = false;
        if (window.MfxDialog && typeof window.MfxDialog.showConfirm === 'function') {
          confirmed = await window.MfxDialog.showConfirm({
            title: t.dialog_title_confirm || 'Please confirm',
            message: t.confirm_reset,
            okText: t.dialog_btn_confirm_reset || 'Reset',
            cancelText: t.dialog_btn_cancel || 'Cancel'
          });
        } else {
          confirmed = confirm(t.confirm_reset);
        }
        if (!confirmed) return;
        core.setStatus(core.statusText('status_resetting', 'Resetting...'));
        const res = await core.apiPost('/api/reset', {});
        if (!res.ok) throw new Error(res.error || 'reset failed');
        await core.reload();
      } catch (e) {
        if (e && e.code === 'unauthorized') return;
        core.setStatus(core.statusError('status_reset_failed', 'Reset failed: ', e), 'warn');
      }
    }

    async function handleStopAction() {
      try {
        if (core.blockActionWhenDisconnected()) return;
        const res = await core.apiPost('/api/stop', {});
        if (!res.ok) throw new Error(res.error || 'stop failed');
        core.markConnection('stopped');
      } catch (e) {
        if (e && e.code === 'unauthorized') return;
        core.setStatus(core.statusError('status_stop_failed', 'Stop failed: ', e), 'warn');
      }
    }

    return {
      handleGeneralAction,
      handleReloadAction,
      handleResetAction,
      handleSaveAction,
      handleStopAction,
      handleWasmAction,
    };
  }

  window.MfxAppActions = { init };
})();
