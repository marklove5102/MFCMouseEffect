<script>
  export let rowEnabled = true;
  export let scopeOptions = [];
  export let scopeMode = 'all';
  export let scopeApps = [];
  export let scopeTitleText = '已选应用';
  export let scopeEmptyText = '已选应用会显示在这里';
  export let scopeEmptyHint = '从右侧应用库中选择或搜索应用';
  export let scopeRemoveAppText = '移除';
  export let onScopeModeChange = null;
  export let onRemoveScopeApp = null;

  function callHandler(handler, ...args) {
    if (typeof handler === 'function') {
      handler(...args);
    }
  }
</script>

<div class="automation-scope-group automation-col">
  <div class="automation-scope-header">
    <h4 class="automation-scope-title">{scopeTitleText}</h4>
    <select
      class="automation-scope-select"
      disabled={!rowEnabled}
      value={scopeMode}
      on:change={(event) => callHandler(onScopeModeChange, event)}
    >
      {#each scopeOptions as option (option.value)}
        <option value={option.value}>{option.label}</option>
      {/each}
    </select>
  </div>
  {#if scopeMode === 'selected'}
    {#if scopeApps.length === 0}
      <div class="automation-scope-empty" role="status">
        <div class="automation-scope-empty-icon">
          <svg viewBox="0 0 24 24" aria-hidden="true" focusable="false">
            <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z" fill="currentColor"/>
          </svg>
        </div>
        <div class="automation-scope-empty-text">{scopeEmptyText}</div>
        <div class="automation-scope-empty-hint">{scopeEmptyHint}</div>
      </div>
    {:else}
      <div class="automation-scope-chip-list">
        {#each scopeApps as app (app)}
          <span class="automation-scope-chip">
            <span class="automation-scope-chip-icon">
              <svg viewBox="0 0 24 24" aria-hidden="true" focusable="false">
                <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z" fill="currentColor"/>
              </svg>
            </span>
            <span class="automation-scope-chip-text">{app}</span>
            <button
              type="button"
              class="automation-scope-chip-remove"
              disabled={!rowEnabled}
              on:click={() => callHandler(onRemoveScopeApp, app)}
              aria-label={`${scopeRemoveAppText} ${app}`}
              title={`${scopeRemoveAppText} ${app}`}
            >
              &times;
            </button>
          </span>
        {/each}
      </div>
    {/if}
  {/if}
</div>
