<script>
  export let rowId = '';
  export let row = {};
  export let rowEnabled = true;
  export let texts = {};
  export let captureTargetKeys = 'keys';
  export let isCapturing = () => false;
  export let actionShortcutInputId = () => '';
  export let onActionShortcutKeydown = null;
  export let onActionShortcutInput = null;
  export let onToggleActionRecord = null;
  export let onActionTypeChange = null;
  export let onActionDelayInput = null;
  export let onActionUrlInput = null;
  export let onActionAppPathInput = null;
  export let onActionMoveUp = null;
  export let onActionMoveDown = null;
  export let onActionRemove = null;
  export let onAddShortcutAction = null;
  export let onAddDelayAction = null;
  export let onAddOpenUrlAction = null;
  export let onAddLaunchAppAction = null;

  function callHandler(handler, ...args) {
    if (typeof handler === 'function') {
      handler(...args);
    }
  }

  function actionsOfRow() {
    return Array.isArray(row?.actions) ? row.actions : [];
  }

  function actionTypeOf(action) {
    return `${action?.type || 'send_shortcut'}`.trim().toLowerCase() || 'send_shortcut';
  }
</script>

<div class="automation-col automation-action-list">
  <div class="automation-shortcut-label">{texts.actionsLabel}</div>

  {#if actionsOfRow().length === 0}
    <div class="automation-action-empty">{texts.actionsEmpty}</div>
  {/if}

  {#each actionsOfRow() as action, index (`${action.type || 'send_shortcut'}-${index}`)}
    <div class="automation-action-card">
      <div class="automation-action-card-head">
        <span class="automation-action-index">{texts.actionLabel} {index + 1}</span>
        <div class="automation-action-card-tools">
          <button
            class="btn-soft automation-action-tool"
            type="button"
            disabled={!rowEnabled || index === 0}
            on:click={() => callHandler(onActionMoveUp, index)}
          >
            {texts.moveUp}
          </button>
          <button
            class="btn-soft automation-action-tool"
            type="button"
            disabled={!rowEnabled || index >= actionsOfRow().length - 1}
            on:click={() => callHandler(onActionMoveDown, index)}
          >
            {texts.moveDown}
          </button>
          <button
            class="btn-soft automation-action-tool"
            type="button"
            disabled={!rowEnabled}
            on:click={() => callHandler(onActionRemove, index)}
          >
            {texts.remove}
          </button>
        </div>
      </div>

      <div class="automation-shortcut-label">{texts.actionTypeLabel}</div>
      <select
        class="automation-modifier-mode automation-action-type-select"
        disabled={!rowEnabled}
        value={actionTypeOf(action)}
        on:change={(event) => callHandler(onActionTypeChange, index, event.currentTarget.value)}
      >
        <option value="send_shortcut">{texts.actionTypeShortcut}</option>
        <option value="delay">{texts.actionTypeDelay}</option>
        <option value="open_url">{texts.actionTypeOpenUrl}</option>
        <option value="launch_app">{texts.actionTypeLaunchApp}</option>
      </select>

      {#if actionTypeOf(action) === 'send_shortcut'}
        <div class="automation-shortcut-label">{texts.shortcutLabel}</div>
        <div class="automation-shortcut-head">
          <input
            id={actionShortcutInputId(index)}
            class="automation-keys"
            type="text"
            disabled={!rowEnabled}
            readonly={isCapturing(rowId, `${captureTargetKeys}:${index}`)}
            value={action.shortcut || ''}
            placeholder={texts.shortcutPlaceholder}
            on:keydown={(event) => callHandler(onActionShortcutKeydown, index, event)}
            on:input={(event) => callHandler(onActionShortcutInput, index, event)}
          />
          <button
            class="btn-soft automation-record"
            class:is-recording={isCapturing(rowId, `${captureTargetKeys}:${index}`)}
            type="button"
            disabled={!rowEnabled}
            on:click={() => callHandler(onToggleActionRecord, index)}
          >
            {isCapturing(rowId, `${captureTargetKeys}:${index}`) ? (texts.recordStop || texts.recording) : texts.record}
          </button>
        </div>
      {:else if actionTypeOf(action) === 'delay'}
        <div class="automation-shortcut-label">{texts.actionDelayLabel}</div>
        <input
          class="automation-keys"
          type="number"
          min="1"
          max="60000"
          step="1"
          disabled={!rowEnabled}
          value={action.delay_ms || ''}
          placeholder={texts.actionDelayPlaceholder}
          on:input={(event) => callHandler(onActionDelayInput, index, event)}
        />
        <div class="automation-action-hint">{texts.actionDelayHint}</div>
      {:else if actionTypeOf(action) === 'open_url'}
        <div class="automation-shortcut-label">{texts.actionUrlLabel}</div>
        <input
          class="automation-keys"
          type="text"
          disabled={!rowEnabled}
          value={action.url || ''}
          placeholder={texts.actionUrlPlaceholder}
          on:input={(event) => callHandler(onActionUrlInput, index, event)}
        />
      {:else if actionTypeOf(action) === 'launch_app'}
        <div class="automation-shortcut-label">{texts.actionAppPathLabel}</div>
        <input
          class="automation-keys"
          type="text"
          disabled={!rowEnabled}
          value={action.app_path || ''}
          placeholder={texts.actionAppPathPlaceholder}
          on:input={(event) => callHandler(onActionAppPathInput, index, event)}
        />
      {/if}
    </div>
  {/each}

  <div class="automation-action-adders">
    <button class="btn-soft" type="button" disabled={!rowEnabled} on:click={() => callHandler(onAddShortcutAction)}>
      {texts.addShortcutAction}
    </button>
    <button class="btn-soft" type="button" disabled={!rowEnabled} on:click={() => callHandler(onAddDelayAction)}>
      {texts.addDelayAction}
    </button>
    <button class="btn-soft" type="button" disabled={!rowEnabled} on:click={() => callHandler(onAddOpenUrlAction)}>
      {texts.addOpenUrlAction}
    </button>
    <button class="btn-soft" type="button" disabled={!rowEnabled} on:click={() => callHandler(onAddLaunchAppAction)}>
      {texts.addLaunchAppAction}
    </button>
  </div>
</div>

<style>
  .automation-action-list {
    gap: 10px;
  }

  .automation-action-empty {
    font-size: 12px;
    color: rgba(255, 255, 255, 0.68);
  }

  .automation-action-card {
    display: grid;
    gap: 8px;
    padding: 10px;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 10px;
    background: rgba(255, 255, 255, 0.03);
  }

  .automation-action-card-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
  }

  .automation-action-card-tools {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
  }

  .automation-action-tool {
    min-width: 54px;
  }

  .automation-action-index {
    font-size: 12px;
    font-weight: 600;
    color: rgba(255, 255, 255, 0.78);
  }

  .automation-action-type-select {
    width: 100%;
  }

  .automation-action-hint {
    font-size: 12px;
    color: rgba(255, 255, 255, 0.62);
  }

  .automation-action-adders {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
  }
</style>
