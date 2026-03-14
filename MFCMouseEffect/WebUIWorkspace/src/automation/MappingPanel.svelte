<script>
  import { createEventDispatcher, onDestroy } from 'svelte';
  import MappingScopePanel from './MappingScopePanel.svelte';
  import MappingShortcutPanel from './MappingShortcutPanel.svelte';
  import { filterCatalogEntries, normalizeCatalogEntries } from './app-catalog.js';
  import {
    pollShortcutCapture,
    readAutomationAppCatalog,
    startShortcutCapture,
    stopShortcutCapture,
  } from './shortcut-capture-remote.js';
  import { shortcutFromKeyboardEvent } from './shortcuts.js';
  import {
    CAPTURE_TARGET_KEYS,
    CAPTURE_TARGET_MODIFIERS,
    buildModifierPayloadFromShortcut,
    chainForRow as buildChainForRow,
    catalogMetaText as buildCatalogMetaText,
    gestureButtonForRow as buildGestureButtonForRow,
    gestureSummaryForRow as buildGestureSummaryForRow,
    isModifierOnlyShortcut,
    modifierInputPlaceholderForRow as buildModifierInputPlaceholderForRow,
    modifierInputValueForRow as buildModifierInputValueForRow,
    modifierShortcutTextForRow as buildModifierShortcutTextForRow,
    normalizedPlatform as resolveNormalizedPlatform,
    scopeAppsForRow as resolveScopeAppsForRow,
    scopeDraftForRow as resolveScopeDraftForRow,
    scopeFileAccept as resolveScopeFileAccept,
    scopeModeForRow as resolveScopeModeForRow,
    scopeSummaryForRow as buildScopeSummaryForRow,
    shortcutSummaryForRow as buildShortcutSummaryForRow,
    triggerValueFromEvent as buildTriggerValueFromEvent,
  } from './mapping-panel-helpers.js';

  export let kind = 'mouse';
  export let rows = [];
  export let options = [];
  export let scopeOptions = [];
  export let gestureButtonOptions = [];
  export let templateValue = '';
  export let templateOptions = [];
  export let texts = {};
  export let platform = 'windows';

  const dispatch = createEventDispatcher();
  let recordingRowId = '';
  let recordingTarget = CAPTURE_TARGET_KEYS;
  let remoteCaptureSessionId = '';
  let remoteCapturePollTimer = 0;
  let localCaptureCommitTimer = 0;
  let localCapturePending = null;
  let globalShortcutSuppressionBound = false;
  let appCatalogEntries = [];
  let appCatalogLoaded = false;
  let appCatalogLoading = false;
  let appCatalogError = '';
  let scopeFilePicker = null;
  let scopeFileRowId = '';

  function normalizedPlatform() {
    return resolveNormalizedPlatform(platform);
  }

  function scopeFileAccept() {
    return resolveScopeFileAccept(platform);
  }

  function isCapturing(rowId, target = CAPTURE_TARGET_KEYS) {
    return recordingRowId === rowId && recordingTarget === target;
  }

  function clearRemotePollTimer() {
    if (!remoteCapturePollTimer) {
      return;
    }
    window.clearTimeout(remoteCapturePollTimer);
    remoteCapturePollTimer = 0;
  }

  function clearLocalCapturePending() {
    if (localCaptureCommitTimer) {
      window.clearTimeout(localCaptureCommitTimer);
      localCaptureCommitTimer = 0;
    }
    localCapturePending = null;
  }

  function onGlobalCaptureShortcut(event) {
    if (!recordingRowId) {
      return;
    }
    if (!event) {
      return;
    }
    event.preventDefault();
    if (typeof event.stopImmediatePropagation === 'function') {
      event.stopImmediatePropagation();
    }
    if (typeof event.stopPropagation === 'function') {
      event.stopPropagation();
    }
  }

  function bindGlobalShortcutSuppression() {
    if (globalShortcutSuppressionBound || typeof window === 'undefined') {
      return;
    }
    window.addEventListener('keydown', onGlobalCaptureShortcut, true);
    window.addEventListener('keyup', onGlobalCaptureShortcut, true);
    window.addEventListener('keypress', onGlobalCaptureShortcut, true);
    globalShortcutSuppressionBound = true;
  }

  function unbindGlobalShortcutSuppression() {
    if (!globalShortcutSuppressionBound || typeof window === 'undefined') {
      return;
    }
    window.removeEventListener('keydown', onGlobalCaptureShortcut, true);
    window.removeEventListener('keyup', onGlobalCaptureShortcut, true);
    window.removeEventListener('keypress', onGlobalCaptureShortcut, true);
    globalShortcutSuppressionBound = false;
  }

  async function endRemoteCapture(sessionId = remoteCaptureSessionId) {
    clearRemotePollTimer();
    if (!sessionId) {
      return;
    }
    if (remoteCaptureSessionId === sessionId) {
      remoteCaptureSessionId = '';
    }
    try {
      await stopShortcutCapture(sessionId);
    } catch (_error) {
      // Keep UI responsive even if server session already ended.
    }
  }

  function activeCaptureInputId(rowId, target = recordingTarget) {
    if (target === CAPTURE_TARGET_MODIFIERS) {
      return modifierInputId(rowId);
    }
    return shortcutInputId(rowId);
  }

  function stopRecording(rowId, blurInput = false) {
    if (recordingRowId !== rowId) {
      return;
    }
    clearLocalCapturePending();
    const target = recordingTarget;
    recordingRowId = '';
    recordingTarget = CAPTURE_TARGET_KEYS;
    if (blurInput) {
      const input = document.getElementById(activeCaptureInputId(rowId, target));
      if (input) {
        input.blur();
      }
    }
  }

  function scheduleRemotePoll(rowId, target, sessionId) {
    clearRemotePollTimer();
    remoteCapturePollTimer = window.setTimeout(() => {
      void pollRemoteCapture(rowId, target, sessionId);
    }, 120);
  }

  async function pollRemoteCapture(rowId, target, sessionId) {
    if (recordingRowId !== rowId || recordingTarget !== target || remoteCaptureSessionId !== sessionId) {
      return;
    }

    try {
      const result = await pollShortcutCapture(sessionId);
      if (recordingRowId !== rowId || recordingTarget !== target || remoteCaptureSessionId !== sessionId) {
        return;
      }

      if (result.status === 'captured' && result.shortcut) {
        clearLocalCapturePending();
        if (target === CAPTURE_TARGET_MODIFIERS) {
          applyTriggerModifierShortcut(rowId, result.shortcut);
        } else {
          emitRowChange(rowId, 'keys', result.shortcut);
        }
        remoteCaptureSessionId = '';
        stopRecording(rowId, true);
        return;
      }

      if (result.status === 'expired' || result.status === 'invalid') {
        remoteCaptureSessionId = '';
        stopRecording(rowId, true);
        return;
      }

      scheduleRemotePoll(rowId, target, sessionId);
    } catch (_error) {
      // Fallback to local keydown capture if remote polling fails.
      remoteCaptureSessionId = '';
      clearRemotePollTimer();
    }
  }

  async function beginRemoteCapture(rowId, target = CAPTURE_TARGET_KEYS) {
    try {
      const sessionId = await startShortcutCapture(10000);
      if (!sessionId || recordingRowId !== rowId || recordingTarget !== target) {
        return;
      }
      remoteCaptureSessionId = sessionId;
      scheduleRemotePoll(rowId, target, sessionId);
    } catch (_error) {
      // Server capture unavailable. Keep local keydown capture active.
      remoteCaptureSessionId = '';
      clearRemotePollTimer();
    }
  }

  function emitRowChange(rowId, key, value) {
    dispatch('rowchange', { kind, rowId, key, value });
  }

  function emitRemove(rowId) {
    if (recordingRowId === rowId) {
      clearLocalCapturePending();
      recordingRowId = '';
      recordingTarget = CAPTURE_TARGET_KEYS;
      void endRemoteCapture();
    }
    dispatch('remove', { kind, rowId });
  }

  function emitAdd() {
    dispatch('add', { kind });
  }

  function emitTemplateChange(value) {
    dispatch('templatechange', { kind, value });
  }

  function emitApplyTemplate() {
    dispatch('applytemplate', { kind });
  }

  function focusCaptureInput(rowId, target = CAPTURE_TARGET_KEYS) {
    const input = document.getElementById(activeCaptureInputId(rowId, target));
    if (!input) {
      return;
    }
    input.focus();
    input.select();
  }

  function onShortcutKeydown(row, event) {
    if (!row.enabled) {
      return;
    }

    if (!isCapturing(row.id, CAPTURE_TARGET_KEYS)) {
      return;
    }

    event.preventDefault();
    event.stopPropagation();

    const lowered = `${event.key || ''}`.toLowerCase();
    if (lowered === 'escape') {
      clearLocalCapturePending();
      event.currentTarget.blur();
      stopRecording(row.id);
      void endRemoteCapture();
      return;
    }

    if ((lowered === 'backspace' || lowered === 'delete') &&
      !event.ctrlKey &&
      !event.altKey &&
      !event.shiftKey &&
      !event.metaKey) {
      clearLocalCapturePending();
      emitRowChange(row.id, 'keys', '');
      stopRecording(row.id);
      void endRemoteCapture();
      return;
    }

    const shortcut = shortcutFromKeyboardEvent(event, platform);
    if (!shortcut) {
      return;
    }

    if (isModifierOnlyShortcut(shortcut)) {
      clearLocalCapturePending();
      localCapturePending = { rowId: row.id, target: CAPTURE_TARGET_KEYS, shortcut };
      localCaptureCommitTimer = window.setTimeout(() => {
        if (!localCapturePending) {
          return;
        }
        const pending = localCapturePending;
        clearLocalCapturePending();
        emitRowChange(pending.rowId, 'keys', pending.shortcut);
        stopRecording(pending.rowId);
        void endRemoteCapture();
      }, 240);
      return;
    }

    clearLocalCapturePending();
    emitRowChange(row.id, 'keys', shortcut);
    stopRecording(row.id);
    void endRemoteCapture();
  }

  function onShortcutInput(row, event) {
    if (isCapturing(row.id, CAPTURE_TARGET_KEYS)) {
      return;
    }
    emitRowChange(row.id, 'keys', event.currentTarget.value);
  }

  function modifierInputId(rowId) {
    return `auto_trigger_modifiers_${kind}_${rowId}`;
  }

  function applyTriggerModifierFlags(rowId, shortcut, emptyMode = 'any') {
    emitRowChange(rowId, 'modifiers', buildModifierPayloadFromShortcut(shortcut, emptyMode));
  }

  function applyTriggerModifierShortcut(rowId, shortcut) {
    applyTriggerModifierFlags(rowId, shortcut, 'none');
  }

  function modifierShortcutTextForRow(row) {
    return buildModifierShortcutTextForRow(row, texts, platform);
  }

  function modifierInputValueForRow(row) {
    return buildModifierInputValueForRow(row, texts, platform);
  }

  function modifierInputPlaceholderForRow(row) {
    return buildModifierInputPlaceholderForRow(row, texts, platform);
  }

  function onTriggerModifierKeydown(row, event) {
    if (!row.enabled) {
      return;
    }
    if (!isCapturing(row.id, CAPTURE_TARGET_MODIFIERS)) {
      return;
    }

    event.preventDefault();
    event.stopPropagation();

    const lowered = `${event.key || ''}`.toLowerCase();
    if (lowered === 'escape') {
      clearLocalCapturePending();
      event.currentTarget.blur();
      stopRecording(row.id);
      void endRemoteCapture();
      return;
    }

    if ((lowered === 'backspace' || lowered === 'delete') &&
      !event.ctrlKey &&
      !event.altKey &&
      !event.shiftKey &&
      !event.metaKey) {
      clearLocalCapturePending();
      emitRowChange(row.id, 'modifiers', {
        mode: 'none',
        primary: false,
        shift: false,
        alt: false,
      });
      stopRecording(row.id);
      void endRemoteCapture();
      return;
    }

    const shortcut = shortcutFromKeyboardEvent(event, platform);
    if (!shortcut) {
      return;
    }

    if (isModifierOnlyShortcut(shortcut)) {
      clearLocalCapturePending();
      localCapturePending = { rowId: row.id, target: CAPTURE_TARGET_MODIFIERS, shortcut };
      localCaptureCommitTimer = window.setTimeout(() => {
        if (!localCapturePending) {
          return;
        }
        const pending = localCapturePending;
        clearLocalCapturePending();
        applyTriggerModifierShortcut(pending.rowId, pending.shortcut);
        stopRecording(pending.rowId);
        void endRemoteCapture();
      }, 240);
      return;
    }

    clearLocalCapturePending();
    applyTriggerModifierShortcut(row.id, shortcut);
    stopRecording(row.id);
    void endRemoteCapture();
  }

  function onTriggerModifierInput(row, event) {
    if (isCapturing(row.id, CAPTURE_TARGET_MODIFIERS)) {
      return;
    }
    applyTriggerModifierFlags(row.id, event.currentTarget.value, 'none');
  }

  function chainForRow(row) {
    return buildChainForRow(row, options);
  }

  function triggerValueFromEvent(event, fallbackTrigger) {
    return buildTriggerValueFromEvent(event, fallbackTrigger, options);
  }

  function onChainChange(row, event) {
    const nextTrigger = triggerValueFromEvent(event, row?.trigger || row?.triggerChain || '');
    if (!nextTrigger) {
      return;
    }
    emitRowChange(row.id, 'triggerChain', nextTrigger);
  }

  function scopeModeForRow(row) {
    return resolveScopeModeForRow(row);
  }

  function scopeAppsForRow(row) {
    return resolveScopeAppsForRow(row);
  }

  function scopeDraftForRow(row) {
    return resolveScopeDraftForRow(row);
  }

  function onScopeModeChange(row, event) {
    emitRowChange(row.id, 'appScopeMode', event.currentTarget.value);
  }

  function onScopeDraftInput(row, event) {
    emitRowChange(row.id, 'appScopeDraft', event.currentTarget.value);
  }

  function onScopeDraftKeydown(row, event) {
    if (event.key !== 'Enter') {
      return;
    }
    event.preventDefault();
    const entries = catalogEntriesForRow(row);
    if (entries.length === 0) {
      return;
    }
    addCatalogScopeApp(row, entries[0].exe);
    emitRowChange(row.id, 'appScopeDraft', '');
  }

  function removeScopeApp(row, app) {
    emitRowChange(row.id, 'appScopeRemove', app);
  }

  function catalogEntriesForRow(row) {
    return filterCatalogEntries(
      appCatalogEntries,
      scopeDraftForRow(row),
      scopeAppsForRow(row),
      120,
      normalizedPlatform());
  }

  function catalogMetaText(entry) {
    return buildCatalogMetaText(entry);
  }

  function addCatalogScopeApp(row, processName) {
    if (!row || !row.id || !row.enabled) {
      return;
    }
    emitRowChange(row.id, 'appScopeAdd', processName);
  }

  function onScopeFilePick(row) {
    if (!row || !row.id || !row.enabled || !scopeFilePicker) {
      return;
    }
    scopeFileRowId = row.id;
    scopeFilePicker.value = '';
    scopeFilePicker.click();
  }

  function onScopeFileChange(event) {
    const input = event.currentTarget;
    const rowId = scopeFileRowId;
    scopeFileRowId = '';
    const files = Array.from(input?.files || []);
    if (!rowId || files.length === 0) {
      if (input) {
        input.value = '';
      }
      return;
    }

    for (const file of files) {
      const name = `${file?.name || ''}`.trim();
      if (!name) {
        continue;
      }
      emitRowChange(rowId, 'appScopeAdd', name);
    }
    if (input) {
      input.value = '';
    }
  }

  async function loadAppCatalog(force = false) {
    if (appCatalogLoading) {
      return;
    }
    appCatalogLoading = true;
    appCatalogError = '';
    try {
      const apps = await readAutomationAppCatalog(force);
      appCatalogEntries = normalizeCatalogEntries(apps, normalizedPlatform());
      appCatalogLoaded = true;
    } catch (_error) {
      appCatalogError = texts.scopeCatalogLoadFailed || 'Failed to load app list.';
      if (!appCatalogLoaded) {
        appCatalogEntries = [];
      }
    } finally {
      appCatalogLoading = false;
    }
  }

  function onRefreshAppCatalog() {
    void loadAppCatalog(true);
  }

  function toggleRecord(rowId, target = CAPTURE_TARGET_KEYS) {
    if (recordingRowId === rowId && recordingTarget === target) {
      clearLocalCapturePending();
      recordingRowId = '';
      recordingTarget = CAPTURE_TARGET_KEYS;
      const input = document.getElementById(activeCaptureInputId(rowId, target));
      if (input) {
        input.blur();
      }
      void endRemoteCapture();
      return;
    }

    if (recordingRowId && recordingRowId !== rowId) {
      stopRecording(recordingRowId, true);
    }
    if (remoteCaptureSessionId) {
      void endRemoteCapture(remoteCaptureSessionId);
    }

    clearLocalCapturePending();
    recordingRowId = rowId;
    recordingTarget = target;
    focusCaptureInput(rowId, target);
    void beginRemoteCapture(rowId, target);
  }

  function shortcutInputId(rowId) {
    return `auto_keys_${kind}_${rowId}`;
  }

  function onRowToggle(rowId, event) {
    const details = event?.currentTarget;
    if (!details || details.open) {
      return;
    }
    if (recordingRowId === rowId) {
      clearLocalCapturePending();
      stopRecording(rowId, true);
      void endRemoteCapture();
    }
  }

  function scopeSummaryForRow(row) {
    return buildScopeSummaryForRow(row, texts);
  }

  function shortcutSummaryForRow(row) {
    return buildShortcutSummaryForRow(row, texts);
  }

  function gestureButtonForRow(row) {
    return buildGestureButtonForRow(row, gestureButtonOptions);
  }

  function gestureSummaryForRow(row) {
    return buildGestureSummaryForRow(row, { kind, options, texts, gestureButtonOptions });
  }

  $: {
    if (recordingRowId) {
      const activeRow = rows.find((item) => item.id === recordingRowId);
      if (activeRow && activeRow.enabled) {
        // Active capture row still valid.
      } else {
        const staleId = recordingRowId;
        const staleTarget = recordingTarget;
        clearLocalCapturePending();
        recordingRowId = '';
        recordingTarget = CAPTURE_TARGET_KEYS;
        void endRemoteCapture();
        const input = document.getElementById(activeCaptureInputId(staleId, staleTarget));
        if (input) {
          input.blur();
        }
      }
    }
  }

  $: {
    const hasSelectedScopeRow = rows.some((row) => scopeModeForRow(row) === 'selected');
    if (hasSelectedScopeRow && !appCatalogLoaded && !appCatalogLoading) {
      void loadAppCatalog(false);
    }
  }

  onDestroy(() => {
    clearLocalCapturePending();
    unbindGlobalShortcutSuppression();
    clearRemotePollTimer();
    if (remoteCaptureSessionId) {
      void endRemoteCapture(remoteCaptureSessionId);
    }
  });

  $: {
    if (recordingRowId) {
      bindGlobalShortcutSuppression();
    } else {
      unbindGlobalShortcutSuppression();
    }
  }
</script>

<div class="automation-panel">
  <div class="automation-list">
    {#if rows.length === 0}
      <div class="automation-empty">{texts.empty}</div>
    {/if}
    {#each rows as row (row.id)}
      <div
        class="automation-row"
        class:is-disabled={!row.enabled}
        class:is-conflict={row.hasConflict}
      >
        <button
          class="btn-soft automation-remove-corner"
          type="button"
          on:click={() => emitRemove(row.id)}
          title={texts.remove}
          aria-label={texts.remove}
        >
          x
        </button>
        <input
          class="automation-toggle"
          type="checkbox"
          checked={row.enabled}
          title={texts.enabledTitle}
          on:change={(event) => emitRowChange(row.id, 'enabled', event.currentTarget.checked)}
        />
        <details class="automation-collapse" on:toggle={(event) => onRowToggle(row.id, event)}>
          <summary
            class="automation-row-head"
            title={texts.expand || 'Expand'}
            aria-label={texts.expand || 'Expand'}
          >
            <span class="automation-row-head-icon" aria-hidden="true"></span>
            <span class="automation-row-head-main">{gestureSummaryForRow(row)}</span>
            <span class="automation-row-head-badges">
              <span class="automation-row-head-meta">{scopeSummaryForRow(row)}</span>
              {#if kind === 'gesture'}
                <span class="automation-row-head-meta">{modifierShortcutTextForRow(row)}</span>
              {/if}
              <span class="automation-row-head-meta">{shortcutSummaryForRow(row)}</span>
            </span>
          </summary>
          <div class="automation-row-body" class:is-scope-all={scopeModeForRow(row) !== 'selected'}>
            <MappingShortcutPanel
              rowId={row.id}
              {row}
              rowEnabled={row.enabled}
              rowKeys={row.keys}
              {kind}
              {texts}
              captureTargetKeys={CAPTURE_TARGET_KEYS}
              captureTargetModifiers={CAPTURE_TARGET_MODIFIERS}
              modifierInputId={modifierInputId(row.id)}
              shortcutInputId={shortcutInputId(row.id)}
              modifierInputValue={modifierInputValueForRow(row)}
              modifierInputPlaceholder={modifierInputPlaceholderForRow(row)}
              gestureButtonValue={gestureButtonForRow(row)}
              {gestureButtonOptions}
              triggerOptions={options}
              chainValue={chainForRow(row)}
              {isCapturing}
              onModifierKeydown={(event) => onTriggerModifierKeydown(row, event)}
              onModifierInput={(event) => onTriggerModifierInput(row, event)}
              onShortcutKeydown={(event) => onShortcutKeydown(row, event)}
              onShortcutInput={(event) => onShortcutInput(row, event)}
              onToggleRecord={(target) => toggleRecord(row.id, target)}
              onTriggerButtonChange={(event) => emitRowChange(row.id, 'triggerButton', event.currentTarget.value)}
              onGestureTriggerChange={(event) => emitRowChange(row.id, 'triggerChain', event.detail?.trigger)}
              onGesturePatternChange={(event) => emitRowChange(row.id, 'gesturePattern', event.detail?.gesturePattern)}
              onChainChange={(event) => onChainChange(row, event)}
            />
            <MappingScopePanel
              rowEnabled={row.enabled}
              scopeOptions={scopeOptions}
              scopeMode={scopeModeForRow(row)}
              scopeApps={scopeAppsForRow(row)}
              scopeDraft={scopeDraftForRow(row)}
              {texts}
              catalogEntries={catalogEntriesForRow(row)}
              appCatalogLoading={appCatalogLoading}
              appCatalogError={appCatalogError}
              onScopeModeChange={(event) => onScopeModeChange(row, event)}
              onScopeDraftInput={(event) => onScopeDraftInput(row, event)}
              onScopeDraftKeydown={(event) => onScopeDraftKeydown(row, event)}
              onRemoveScopeApp={(app) => removeScopeApp(row, app)}
              onAddCatalogScopeApp={(app) => addCatalogScopeApp(row, app)}
              onRefreshAppCatalog={onRefreshAppCatalog}
              onPickFile={() => onScopeFilePick(row)}
              catalogMetaText={catalogMetaText}
            />
          </div>
        </details>
        <div class="automation-note" class:is-visible={!!row.note}>{row.note}</div>
      </div>
    {/each}
  </div>

  {#if recordingRowId}
    <div class="automation-capture-hint is-active">
      {recordingTarget === CAPTURE_TARGET_MODIFIERS
        ? (texts.captureHintModifierActive || texts.captureHintActive)
        : texts.captureHintActive}
    </div>
  {/if}

  <div class="automation-actions">
    <button class="btn-soft" type="button" on:click={emitAdd}>
      {texts.add}
    </button>
    <select
      class="automation-template-select"
      value={templateValue}
      title={texts.templateTitle}
      on:change={(event) => emitTemplateChange(event.currentTarget.value)}
    >
      <option value="">{texts.templateNone}</option>
      {#each templateOptions as option (option.id)}
        <option value={option.id}>{option.label}</option>
      {/each}
    </select>
    <button class="btn-soft" type="button" disabled={!templateValue} on:click={emitApplyTemplate}>
      {texts.applyTemplate}
    </button>
  </div>

  <input
    class="automation-scope-file-input"
    type="file"
    accept={scopeFileAccept()}
    multiple
    bind:this={scopeFilePicker}
    on:change={onScopeFileChange}
  />
</div>
