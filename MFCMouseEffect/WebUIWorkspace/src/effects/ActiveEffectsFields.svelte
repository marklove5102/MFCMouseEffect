<script>
  import { createEventDispatcher } from 'svelte';

  export let clickOptions = [];
  export let trailOptions = [];
  export let scrollOptions = [];
  export let holdOptions = [];
  export let hoverOptions = [];
  export let effectCapabilities = {};
  export let active = {};

  const dispatch = createEventDispatcher();

  function normalizeActive(input) {
    const value = input || {};
    return {
      click: value.click || '',
      trail: value.trail || '',
      scroll: value.scroll || '',
      hold: value.hold || '',
      hover: value.hover || '',
    };
  }

  function toSnapshot(form) {
    const value = form || normalizeActive({});
    return {
      click: value.click || '',
      trail: value.trail || '',
      scroll: value.scroll || '',
      hold: value.hold || '',
      hover: value.hover || '',
    };
  }

  function isSupported(key) {
    const value = effectCapabilities || {};
    return value[key] !== false;
  }

  const effectKeys = ['click', 'trail', 'scroll', 'hold', 'hover'];
  $: unsupportedEffects = effectKeys.filter((key) => !isSupported(key));

  let form = normalizeActive(active);
  let lastActiveRef = active;

  $: if (active !== lastActiveRef) {
    lastActiveRef = active;
    form = normalizeActive(active);
  }

  $: dispatch('change', toSnapshot(form));
</script>

<div class="grid">
  <label for="click" data-i18n="label_click">Click</label>
  <select id="click" bind:value={form.click} disabled={!isSupported('click')}>
    {#each clickOptions as option}
      <option value={option.value}>{option.label}</option>
    {/each}
  </select>

  <label for="trail" data-i18n="label_trail">Trail</label>
  <select id="trail" bind:value={form.trail} disabled={!isSupported('trail')}>
    {#each trailOptions as option}
      <option value={option.value}>{option.label}</option>
    {/each}
  </select>

  <label for="scroll" data-i18n="label_scroll">Scroll</label>
  <select id="scroll" bind:value={form.scroll} disabled={!isSupported('scroll')}>
    {#each scrollOptions as option}
      <option value={option.value}>{option.label}</option>
    {/each}
  </select>

  <label for="hold" data-i18n="label_hold">Hold</label>
  <select id="hold" bind:value={form.hold} disabled={!isSupported('hold')}>
    {#each holdOptions as option}
      <option value={option.value}>{option.label}</option>
    {/each}
  </select>

  <label for="hover" data-i18n="label_hover">Hover</label>
  <select id="hover" bind:value={form.hover} disabled={!isSupported('hover')}>
    {#each hoverOptions as option}
      <option value={option.value}>{option.label}</option>
    {/each}
  </select>

  {#if unsupportedEffects.length > 0}
    <div class="hint span2" data-i18n="hint_effect_capability_limited">
      Some effect categories are unavailable on this platform and have been disabled.
    </div>
  {/if}
</div>
