<script>
  export let rowId = '';
  export let row = {};
  export let rowEnabled = true;
  export let rowKeys = '';
  export let kind = 'mouse';
  export let texts = {};
  export let captureTargetKeys = 'keys';
  export let captureTargetModifiers = 'modifiers';
  export let modifierInputId = '';
  export let shortcutInputId = '';
  export let modifierInputValue = '';
  export let modifierInputPlaceholder = '';
  export let gestureButtonValue = 'right';
  export let gestureButtonOptions = [];
  export let triggerOptions = [];
  export let chainValue = [];
  export let isCapturing = () => false;
  export let onModifierKeydown = null;
  export let onModifierInput = null;
  export let onShortcutKeydown = null;
  export let onShortcutInput = null;
  export let onToggleRecord = null;
  export let onTriggerButtonChange = null;
  export let onGestureTriggerChange = null;
  export let onGesturePatternChange = null;
  export let onChainChange = null;

  import GesturePatternEditor from './GesturePatternEditor.svelte';
  import TriggerChainEditor from './TriggerChainEditor.svelte';

  function callHandler(handler, ...args) {
    if (typeof handler === 'function') {
      handler(...args);
    }
  }
</script>

<div class="automation-chain-group automation-col">
  {#if kind === 'gesture'}
    <div class="automation-shortcut-label">{texts.gestureTriggerShortcutLabel}</div>
    <div class="automation-shortcut-head">
      <input
        id={modifierInputId}
        class="automation-keys"
        type="text"
        disabled={!rowEnabled}
        readonly={isCapturing(rowId, captureTargetModifiers)}
        value={modifierInputValue}
        placeholder={modifierInputPlaceholder}
        on:keydown={(event) => callHandler(onModifierKeydown, event)}
        on:input={(event) => callHandler(onModifierInput, event)}
      />
      <button
        class="btn-soft automation-record"
        class:is-recording={isCapturing(rowId, captureTargetModifiers)}
        type="button"
        disabled={!rowEnabled}
        on:click={() => callHandler(onToggleRecord, captureTargetModifiers)}
      >
        {isCapturing(rowId, captureTargetModifiers) ? (texts.recordStop || texts.recording) : texts.record}
      </button>
    </div>
    <div class="automation-shortcut-label">{texts.gestureOutputShortcutLabel || texts.shortcutLabel}</div>
  {/if}
  <div class="automation-shortcut-head">
    <input
      id={shortcutInputId}
      class="automation-keys"
      type="text"
      disabled={!rowEnabled}
      readonly={isCapturing(rowId, captureTargetKeys)}
      value={rowKeys}
      placeholder={texts.shortcutPlaceholder}
      on:keydown={(event) => callHandler(onShortcutKeydown, event)}
      on:input={(event) => callHandler(onShortcutInput, event)}
    />
    <button
      class="btn-soft automation-record"
      class:is-recording={isCapturing(rowId, captureTargetKeys)}
      type="button"
      disabled={!rowEnabled}
      on:click={() => callHandler(onToggleRecord, captureTargetKeys)}
    >
      {isCapturing(rowId, captureTargetKeys) ? (texts.recordStop || texts.recording) : texts.record}
    </button>
  </div>
  {#if kind === 'gesture'}
    <div class="automation-gesture-trigger-group">
      <div class="automation-modifier-label">{texts.gestureTriggerButtonLabel}</div>
      <select
        class="automation-modifier-mode"
        disabled={!rowEnabled}
        value={gestureButtonValue}
        on:change={(event) => callHandler(onTriggerButtonChange, event)}
      >
        {#each gestureButtonOptions as option (option.value)}
          <option value={option.value}>{option.label}</option>
        {/each}
      </select>
    </div>
    <GesturePatternEditor
      {row}
      disabled={!rowEnabled}
      options={triggerOptions}
      texts={{
        title: texts.gesturePatternTitle,
        modePreset: texts.gestureModePreset,
        modeCustom: texts.gestureModeCustom,
        preset: texts.gesturePatternPreset,
        custom: texts.gesturePatternCustom,
        threshold: texts.gestureThreshold,
        canvasEmpty: texts.gestureCanvasEmpty,
        canvasClear: texts.gestureCanvasClear,
        canvasUndo: texts.gestureCanvasUndo,
        canvasGuide: texts.gestureCanvasGuide,
        canvasLimitHint: texts.gestureCanvasLimitHint,
        canvasLimitBadge: texts.gestureCanvasLimitBadge,
        canvasLimitReached: texts.gestureCanvasLimitReached,
        canvasStrokeCount: texts.gestureCanvasStrokeCount,
        canvasPointUnit: texts.gestureCanvasPointUnit,
        canvasNoDirection: texts.gestureCanvasNoDirection,
        canvasDraw: texts.gestureCanvasDraw,
        canvasSave: texts.gestureCanvasSave,
        canvasLockedHint: texts.gestureCanvasLockedHint,
      }}
      on:triggerchange={(event) => callHandler(onGestureTriggerChange, event)}
      on:change={(event) => callHandler(onGesturePatternChange, event)}
    />
  {:else}
    <TriggerChainEditor
      value={chainValue}
      options={triggerOptions}
      disabled={!rowEnabled}
      texts={{
        addNode: texts.addChainNode,
        removeNode: texts.removeChainNode,
        chainJoiner: texts.chainJoiner,
      }}
      on:chainchange={(event) => callHandler(onChainChange, event)}
    />
  {/if}
</div>
