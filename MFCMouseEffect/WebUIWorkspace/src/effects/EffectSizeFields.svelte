<script>
  import { createEventDispatcher } from "svelte";

  export let scales = {};

  const dispatch = createEventDispatcher();

  const rows = [
    { key: "click", labelKey: "label_click", labelDefault: "Click" },
    { key: "trail", labelKey: "label_trail", labelDefault: "Trail" },
    { key: "scroll", labelKey: "label_scroll", labelDefault: "Scroll" },
    { key: "hold", labelKey: "label_hold", labelDefault: "Hold" },
    { key: "hover", labelKey: "label_hover", labelDefault: "Hover" },
  ];

  function toNumber(value, fallback) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : fallback;
  }

  function clampPercent(value) {
    return Math.min(200, Math.max(50, Math.round(toNumber(value, 100))));
  }

  function normalizeScales(input) {
    const value = input || {};
    return {
      click: clampPercent(value.click),
      trail: clampPercent(value.trail),
      scroll: clampPercent(value.scroll),
      hold: clampPercent(value.hold),
      hover: clampPercent(value.hover),
    };
  }

  function toSnapshot(form) {
    const value = form || normalizeScales({});
    return {
      click: clampPercent(value.click),
      trail: clampPercent(value.trail),
      scroll: clampPercent(value.scroll),
      hold: clampPercent(value.hold),
      hover: clampPercent(value.hover),
    };
  }

  function updateScale(key, value) {
    const next = clampPercent(value);
    if (form[key] === next) {
      return;
    }
    form = {
      ...form,
      [key]: next,
    };
  }

  let form = normalizeScales(scales);
  let lastScalesRef = scales;

  function toSliderFraction(value) {
    return (clampPercent(value) - 50) / 150;
  }

  $: if (scales !== lastScalesRef) {
    lastScalesRef = scales;
    form = normalizeScales(scales);
  }

  $: dispatch("change", toSnapshot(form));
</script>

<div class="grid effect-size-grid">
  {#each rows as row}
    <label
      for={`effect_size_${row.key}`}
      class="effect-size-label"
      data-i18n={row.labelKey}>{row.labelDefault}</label
    >
    <div
      class="effect-size-slider-shell"
      style="--slider-frac:{toSliderFraction(form[row.key])}"
      title="{form[row.key]}% (50%–200%)"
    >
      <div class="effect-size-slider-track" aria-hidden="true">
        <div class="effect-size-slider-fill"></div>
      </div>
      <div class="effect-size-slider-thumb" aria-hidden="true"></div>
      <input
        id={`effect_size_${row.key}`}
        type="range"
        min="50"
        max="200"
        step="5"
        value={form[row.key]}
        aria-label={row.labelDefault}
        aria-valuemin="50"
        aria-valuemax="200"
        aria-valuenow={form[row.key]}
        on:input={(event) => updateScale(row.key, event?.target?.value)}
      />
    </div>
    <input
      id={`effect_size_${row.key}_number`}
      class="pair effect-size-number"
      type="number"
      min="50"
      max="200"
      step="5"
      value={form[row.key]}
      on:input={(event) => updateScale(row.key, event?.target?.value)}
      on:change={(event) => updateScale(row.key, event?.target?.value)}
    />
  {/each}
  <div class="hint effect-size-hint" data-i18n="hint_effect_size_scale">
    100 means default size. Range is 50% to 200%.
  </div>
</div>
