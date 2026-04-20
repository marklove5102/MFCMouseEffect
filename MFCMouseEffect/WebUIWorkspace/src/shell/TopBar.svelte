<script>
  import { createEventDispatcher } from "svelte";

  export let actionsDisabled = false;
  export let statusMessage = "";
  export let statusTone = "";

  const dispatch = createEventDispatcher();
  let statusExpanded = false;

  $: statusCanExpand = `${statusMessage || ""}`.trim().length > 120 || `${statusMessage || ""}`.includes("\n");
  $: if (!statusCanExpand) {
    statusExpanded = false;
  }

  function emitAction(type) {
    dispatch("action", { type });
  }
</script>

<div class="top">
  <div class="top-head">
    <div class="brand">
      <div class="t" data-i18n="title">MFCMouseEffect Settings</div>
      <div class="s" data-i18n="subtitle">
        Saved to config.json. Click Apply to keep changes.
      </div>
    </div>

    <div class="btns">
      <a
        id="btnStar"
        class="btn-link btn-star btn-icon"
        href="https://github.com/sqmw/MFCMouseEffect"
        target="_blank"
        rel="noopener"
        data-i18n-title="tip_star"
        title="Open project page"
      >
        <svg class="btn-icon__glyph" viewBox="0 0 20 20" aria-hidden="true">
          <path d="M10 2.4l2.1 4.28 4.72.69-3.41 3.33.8 4.7L10 13.16 5.78 15.4l.8-4.7-3.41-3.33 4.72-.69L10 2.4z" fill="currentColor"></path>
        </svg>
        <span class="sr-only" data-i18n="btn_star">Star</span>
      </a>
      <button
        id="btnReset"
        class="btn-danger btn-icon"
        data-i18n-title="tip_reset"
        title="Reset to defaults"
        disabled={actionsDisabled}
        on:click={() => emitAction("reset")}
      >
        <svg class="btn-icon__glyph" viewBox="0 0 20 20" aria-hidden="true">
          <path d="M6.3 6.6A5.6 5.6 0 1 1 5 11.4" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"></path>
          <path d="M4.1 5.1h3.9V9" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"></path>
        </svg>
        <span class="sr-only" data-i18n="btn_reset">Reset</span>
      </button>
      <button
        id="btnStop"
        class="btn-soft btn-icon"
        data-i18n-title="tip_stop"
        title="Stop server (reopen from tray)"
        disabled={actionsDisabled}
        on:click={() => emitAction("stop")}
      >
        <svg class="btn-icon__glyph" viewBox="0 0 20 20" aria-hidden="true">
          <rect x="5.2" y="5.2" width="9.6" height="9.6" rx="1.6" fill="currentColor"></rect>
        </svg>
        <span class="sr-only" data-i18n="btn_stop">Stop</span>
      </button>
      <button
        id="btnReload"
        class="btn-soft btn-icon"
        data-i18n-title="tip_reload"
        title="Reload config.json from disk"
        disabled={actionsDisabled}
        on:click={() => emitAction("reload")}
      >
        <svg class="btn-icon__glyph" viewBox="0 0 20 20" aria-hidden="true">
          <path d="M13.7 6.6A5.6 5.6 0 1 0 15 11.4" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"></path>
          <path d="M15.9 5.1h-3.9V9" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"></path>
        </svg>
        <span class="sr-only" data-i18n="btn_reload">Reload</span>
      </button>
      <button
        id="btnSave"
        class="primary"
        data-i18n="btn_apply"
        data-i18n-title="tip_apply"
        title="Apply current form values"
        disabled={actionsDisabled}
        on:click={() => emitAction("save")}
      >
        Apply
      </button>
    </div>
  </div>

  <div class="top-subbar">
    <div
      class="top-status"
      class:show={!!statusMessage}
      class:warn={statusTone === "warn"}
      class:ok={statusTone === "ok"}
      class:offline={statusTone === "offline"}
      class:is-expanded={statusExpanded}
      id="status"
      aria-live="polite"
    >
      <div class="top-status__text">
        {statusMessage || ""}
      </div>
      {#if statusCanExpand}
        <button
          type="button"
          class="btn-soft top-status__toggle"
          on:click={() => { statusExpanded = !statusExpanded; }}
        >
          <span data-i18n={statusExpanded ? "btn_status_collapse" : "btn_status_expand"}>
            {statusExpanded ? "Collapse" : "Expand"}
          </span>
        </button>
      {/if}
    </div>
  </div>
</div>
