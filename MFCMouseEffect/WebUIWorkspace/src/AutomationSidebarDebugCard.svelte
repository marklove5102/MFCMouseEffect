<script>
  import {
    boolLabel,
    formatGestureRouteModifiers,
    gestureRouteSourceLabel,
    normalizeGestureRouteStatus,
    recentActionRuns,
    recentGestureRouteEvents,
    selectLatestGestureEvent,
    selectLatestMatchedGestureEvent,
    selectLatestRecognizedGestureEvent,
  } from './automation/gesture-route-debug-model.js';
  import {
    buildGesturePreviewFromPoints,
    gestureIdDirectionHint,
  } from './automation/gesture-id-preview.js';

  export let payloadState = {};
  export let platform = 'windows';
  export let i18n = {};
  export let compact = false;

  let routeStatus = null;
  let recentEvents = [];
  let latestGestureEvent = null;
  let latestRecognizedGestureEvent = null;
  let latestMatchedGestureEvent = null;
  let recognizedGestureId = '';
  let matchedGestureId = '';
  let heroGesturePreview = null;
  let recognizedGestureCardPreview = null;
  let matchedGestureCardPreview = null;
  let lastRecognizedPreviewPoints = [];
  let lastMatchedPreviewPoints = [];
  let recentActionRunItems = [];
  let recognizedGestureHint = '';
  let matchedGestureHint = '';
  let shouldResetRecognizedPreview = false;

  function t(key, fallback) {
    const value = `${i18n?.[key] || ''}`.trim();
    return value || fallback;
  }

  function shouldResetPreviewByRouteStatus(status) {
    const stage = `${status?.lastStage || ''}`.trim();
    const reason = `${status?.lastReason || ''}`.trim();
    if (stage === 'buttonless_move_skipped') {
      return reason === 'awaiting_first_motion_arm';
    }
    if (stage === 'buttonless_idle_reset' && reason === 'idle_timeout') {
      return true;
    }
    return false;
  }

  function samplePointsEvenly(points, maxPoints = 220) {
    if (!Array.isArray(points) || points.length <= 0) {
      return [];
    }
    const cap = Math.max(24, Math.min(320, Math.trunc(maxPoints)));
    if (points.length <= cap) {
      return points;
    }
    const out = [];
    const step = (points.length - 1) / (cap - 1);
    for (let i = 0; i < cap; i += 1) {
      const index = Math.min(points.length - 1, Math.round(step * i));
      out.push(points[index]);
    }
    return out;
  }

  $: routeStatus = normalizeGestureRouteStatus(payloadState);
  $: recentEvents = recentGestureRouteEvents(routeStatus, compact ? 6 : 8);
  $: latestGestureEvent = selectLatestGestureEvent(routeStatus);
  $: latestRecognizedGestureEvent = selectLatestRecognizedGestureEvent(routeStatus);
  $: latestMatchedGestureEvent = selectLatestMatchedGestureEvent(routeStatus);
  $: recentActionRunItems = recentActionRuns(routeStatus, compact ? 3 : 4);
  $: shouldResetRecognizedPreview = shouldResetPreviewByRouteStatus(routeStatus);
  $: recognizedGestureId = `${routeStatus?.lastRecognizedGestureId || latestRecognizedGestureEvent?.recognizedGestureId || latestGestureEvent?.gestureId || ''}`.trim();
  $: matchedGestureId = `${routeStatus?.lastMatchedGestureId || latestMatchedGestureEvent?.matchedGestureId || ''}`.trim();
  $: {
    const current = routeStatus?.lastPreviewPoints?.length >= 2
      ? routeStatus.lastPreviewPoints
      : (latestRecognizedGestureEvent?.previewPoints?.length >= 2
        ? latestRecognizedGestureEvent.previewPoints
        : []);
    if (shouldResetRecognizedPreview) {
      lastRecognizedPreviewPoints = [];
    } else if (current.length >= 2) {
      lastRecognizedPreviewPoints = current;
    }
  }
  $: {
    const current = latestMatchedGestureEvent?.previewPoints?.length >= 2
      ? latestMatchedGestureEvent.previewPoints
      : [];
    if (current.length >= 2) {
      lastMatchedPreviewPoints = current;
    }
  }
  $: heroGesturePreview = shouldResetRecognizedPreview
    ? null
    : buildGesturePreviewFromPoints(
      samplePointsEvenly(lastRecognizedPreviewPoints, 300),
      { width: 680, height: 380, padding: 18, fitRatio: 0.88 },
    );
  $: recognizedGestureCardPreview = shouldResetRecognizedPreview
    ? null
    : buildGesturePreviewFromPoints(
      samplePointsEvenly(lastRecognizedPreviewPoints, 220),
      { width: 168, height: 88, padding: 8, fitRatio: 0.9 },
    );
  $: matchedGestureCardPreview = buildGesturePreviewFromPoints(
    samplePointsEvenly(lastMatchedPreviewPoints, 220),
    { width: 168, height: 88, padding: 8, fitRatio: 0.9 },
  );
  $: recognizedGestureHint = shouldResetRecognizedPreview ? '' : gestureIdDirectionHint(recognizedGestureId);
  $: matchedGestureHint = gestureIdDirectionHint(matchedGestureId);

  function displayGestureLabel(gestureHint, gestureId) {
    return gestureHint || gestureId || '-';
  }

  function sourceLabel(event) {
    if (event?.usedCustom) {
      return t('sourceCustom', 'Custom');
    }
    if (event?.usedPreset) {
      return t('sourcePreset', 'Preset');
    }
    return t('sourceUnknown', 'Unknown');
  }

  function eventLine(event) {
    if (!event) {
      return '-';
    }
    const stage = event.stage || '-';
    const reason = event.reason || '-';
    return `${stage} · ${reason}`;
  }

  function actionRunLine(run) {
    if (!run) {
      return '-';
    }
    const status = run.status || '-';
    const reason = run.stopReason || '-';
    return `${status} · ${reason}`;
  }

  function actionStepLine(step) {
    if (!step) {
      return '-';
    }
    const type = step.type || '-';
    const target = step.target || '-';
    const status = step.status || '-';
    return `${type} · ${target} · ${status}`;
  }

  function detailsSummary(label, count) {
    if (!count || count <= 0) {
      return label;
    }
    return `${label} (${count})`;
  }

  function truthyTone(value) {
    if (value === true) {
      return 'success';
    }
    if (value === false) {
      return 'danger';
    }
    return 'neutral';
  }
</script>

{#if routeStatus}
  <section class:workspace-debug-card--compact={compact} class="workspace-debug-card" aria-live="polite">
    {#if !compact}
      <div class="workspace-debug-head">
        <div class="workspace-debug-title">{t('label_auto_gesture_debug', '手势实时调试（Debug）')}</div>
      </div>
    {/if}

    <div class="workspace-debug-layout">
      <div class="workspace-debug-canvas-card">
        <div class="workspace-debug-canvas-card__title">
          {t('label_auto_gesture_debug_last_gesture', '识别手势')}
        </div>
        <div class="workspace-debug-canvas-card__subtitle">
          {displayGestureLabel(recognizedGestureHint, recognizedGestureId)}
        </div>
        {#if heroGesturePreview}
          <svg
            class="workspace-debug-canvas"
            viewBox={`0 0 ${heroGesturePreview.width} ${heroGesturePreview.height}`}
            aria-hidden="true"
          >
            <path
              class="workspace-debug-canvas__grid"
              fill="none"
              stroke="rgba(182, 205, 232, 0.42)"
              stroke-width="2"
              stroke-dasharray="12 12"
              d={`M 28 ${(heroGesturePreview.height / 2).toFixed(1)} H ${(heroGesturePreview.width - 28).toFixed(1)} M ${(heroGesturePreview.width / 2).toFixed(1)} 20 V ${(heroGesturePreview.height - 20).toFixed(1)}`}
            />
            <path
              class="workspace-debug-canvas__path"
              fill="none"
              stroke="#2f7ed8"
              stroke-width="14"
              stroke-linecap="round"
              stroke-linejoin="round"
              d={heroGesturePreview.path}
            />
            {#if heroGesturePreview.startPoint}
              <circle
                class="workspace-debug-canvas__start"
                cx={heroGesturePreview.startPoint.x}
                cy={heroGesturePreview.startPoint.y}
                r="18"
                fill="#eef6ff"
                stroke="#2f7ed8"
                stroke-width="8"
              />
            {/if}
            {#if heroGesturePreview.arrowPath}
              <path
                class="workspace-debug-canvas__arrow"
                fill="#eef6ff"
                stroke="#2f7ed8"
                stroke-width="9"
                stroke-linecap="round"
                stroke-linejoin="round"
                d={heroGesturePreview.arrowPath}
              />
            {/if}
          </svg>
        {:else}
          <div class="workspace-debug-canvas workspace-debug-canvas--empty">-</div>
        {/if}
      </div>

      <aside class="workspace-debug-sidebar">
        <div class="workspace-debug-insight-grid">
          <div class="workspace-debug-insight-card">
            <div class="workspace-debug-insight-card__label">
              {t('label_auto_gesture_debug_matched_gesture', '匹配手势')}
            </div>
            <div class="workspace-debug-mini-preview">
              {#if matchedGestureCardPreview}
                <svg
                  class="workspace-debug-mini-preview__svg"
                  viewBox={`0 0 ${matchedGestureCardPreview.width} ${matchedGestureCardPreview.height}`}
                  aria-hidden="true"
                >
                  <path
                    class="workspace-debug-gesture__grid"
                    fill="none"
                    stroke="rgba(157, 186, 219, 0.45)"
                    stroke-width="1"
                    stroke-dasharray="4 4"
                    d={`M 8 ${(matchedGestureCardPreview.height / 2).toFixed(1)} H ${(matchedGestureCardPreview.width - 8).toFixed(1)} M ${(matchedGestureCardPreview.width / 2).toFixed(1)} 8 V ${(matchedGestureCardPreview.height - 8).toFixed(1)}`}
                  />
                  <path
                    class="workspace-debug-gesture__path"
                    fill="none"
                    stroke="#2f7ed8"
                    stroke-width="4"
                    stroke-linecap="round"
                    stroke-linejoin="round"
                    d={matchedGestureCardPreview.path}
                  />
                  {#if matchedGestureCardPreview.startPoint}
                    <circle
                      class="workspace-debug-gesture__start"
                      cx={matchedGestureCardPreview.startPoint.x}
                      cy={matchedGestureCardPreview.startPoint.y}
                      r="5"
                      fill="#ebf4ff"
                      stroke="#2f7ed8"
                      stroke-width="3"
                    />
                  {/if}
                  {#if matchedGestureCardPreview.arrowPath}
                    <path
                      class="workspace-debug-gesture__arrow"
                      fill="#ebf4ff"
                      stroke="#2f7ed8"
                      stroke-width="3"
                      stroke-linecap="round"
                      stroke-linejoin="round"
                      d={matchedGestureCardPreview.arrowPath}
                    />
                  {/if}
                </svg>
              {:else}
                <div class="workspace-debug-mini-preview__empty">-</div>
              {/if}
              <strong title={matchedGestureId || '-'}>
                {displayGestureLabel(matchedGestureHint, matchedGestureId)}
              </strong>
            </div>
          </div>

          <div class="workspace-debug-insight-card">
            <div class="workspace-debug-insight-card__label">
              {t('label_auto_gesture_debug_last_reason', '原因')}
            </div>
            <strong class="workspace-debug-insight-card__value">{routeStatus.lastReason || '-'}</strong>
            <div class="workspace-debug-chip-row">
              <span class="workspace-debug-chip">{t('label_auto_gesture_debug_last_stage', '阶段')} {routeStatus.lastStage || '-'}</span>
            </div>
          </div>
        </div>

        <div class="workspace-debug-chip-row workspace-debug-chip-row--status">
          <span class="workspace-debug-chip">{t('label_auto_gesture_debug_trigger_button', '触发按键')} {routeStatus.lastTriggerButton || '-'}</span>
          <span class:workspace-debug-chip--success={truthyTone(routeStatus.lastMatched) === 'success'} class:workspace-debug-chip--danger={truthyTone(routeStatus.lastMatched) === 'danger'} class="workspace-debug-chip">
            {t('label_auto_gesture_debug_matched', '命中映射')} {boolLabel(routeStatus.lastMatched, i18n)}
          </span>
          <span class:workspace-debug-chip--success={truthyTone(routeStatus.lastInjected) === 'success'} class:workspace-debug-chip--danger={truthyTone(routeStatus.lastInjected) === 'danger'} class="workspace-debug-chip">
            {t('label_auto_gesture_debug_injected', '快捷键已注入')} {boolLabel(routeStatus.lastInjected, i18n)}
          </span>
        </div>

        <div class="workspace-debug-grid">
          <div class="workspace-debug-item">
            <span>{t('label_auto_gesture_debug_source', '匹配来源')}</span>
            <strong>{gestureRouteSourceLabel(routeStatus, i18n)}</strong>
          </div>
          <div class="workspace-debug-item">
            <span>{t('label_auto_gesture_debug_modifiers', '修饰键')}</span>
            <strong>{formatGestureRouteModifiers(routeStatus.modifiers, platform, i18n)}</strong>
          </div>
          <div class="workspace-debug-item">
            <span>{t('label_auto_gesture_debug_candidates', '候选/窗口')}</span>
            <strong>
              {routeStatus.lastCandidateCount || 0}
              ·
              [{routeStatus.lastBestWindowStart ?? -1}, {routeStatus.lastBestWindowEnd ?? -1}]
            </strong>
          </div>
          <div class="workspace-debug-item">
            <span>{t('label_auto_gesture_debug_runner_up', '次优分数')}</span>
            <strong>{routeStatus.lastRunnerUpScore >= 0 ? routeStatus.lastRunnerUpScore.toFixed(1) : '-'}</strong>
          </div>
          <div class="workspace-debug-item workspace-debug-item--wide">
            <span>{t('label_auto_gesture_debug_last_action_run', '最近执行动作')}</span>
            <strong>{routeStatus.lastActionRun ? actionRunLine(routeStatus.lastActionRun) : '-'}</strong>
          </div>
        </div>

        {#if recentEvents.length > 0}
          <details class="workspace-debug-disclosure">
            <summary>{detailsSummary(t('label_auto_gesture_debug_recent_events', '最近事件'), recentEvents.length)}</summary>
            <div class="workspace-debug-events__list">
              {#each recentEvents as event (`event_${event.seq}_${event.timestampMs}`)}
                <div class="workspace-debug-event-row">
                  <span class="workspace-debug-event-row__seq">#{event.seq}</span>
                  <span class="workspace-debug-event-row__line">{eventLine(event)}</span>
                  <span class="workspace-debug-event-row__tag">{sourceLabel(event)}</span>
                  <span class="workspace-debug-event-row__tag">{event.triggerButton || '-'}</span>
                  <span class="workspace-debug-event-row__tag">
                    {boolLabel(event.matched, i18n)}/{boolLabel(event.injected, i18n)}
                  </span>
                </div>
              {/each}
            </div>
          </details>
        {/if}

        {#if recentActionRunItems.length > 0}
          <details class="workspace-debug-disclosure">
            <summary>{detailsSummary(t('label_auto_gesture_debug_recent_action_runs', '最近动作执行'), recentActionRunItems.length)}</summary>
            <div class="workspace-debug-events__list">
              {#each recentActionRunItems as run (`run_${run.seq}_${run.timestampMs}`)}
                <div class="workspace-debug-event-row">
                  <span class="workspace-debug-event-row__seq">#{run.seq}</span>
                  <span class="workspace-debug-event-row__line">{actionRunLine(run)}</span>
                  <span class="workspace-debug-event-row__tag">{boolLabel(run.executed, i18n)}</span>
                </div>
                {#if run.steps.length > 0}
                  <div class="workspace-debug-action-steps">
                    {#each run.steps as step (`step_${run.seq}_${step.index}`)}
                      <div class="workspace-debug-action-step">
                        <span class="workspace-debug-action-step__index">{step.index + 1}.</span>
                        <span class="workspace-debug-action-step__line">{actionStepLine(step)}</span>
                        <span class="workspace-debug-action-step__detail">{step.detail || '-'}</span>
                      </div>
                    {/each}
                  </div>
                {/if}
              {/each}
            </div>
          </details>
        {/if}
      </aside>
    </div>
  </section>
{/if}

<style>
  .workspace-debug-card {
    border: 1px solid rgba(194, 208, 226, 0.86);
    border-radius: 12px;
    background: linear-gradient(180deg, rgba(245, 251, 255, 0.96), rgba(237, 246, 255, 0.94));
    box-shadow: 0 12px 28px rgba(53, 86, 124, 0.08);
    padding: 12px;
  }

  .workspace-debug-card--compact {
    padding: 10px;
    box-shadow: none;
  }

  .workspace-debug-head {
    display: flex;
    flex-wrap: wrap;
    align-items: flex-start;
    justify-content: space-between;
    gap: 8px 12px;
  }

  .workspace-debug-title {
    margin: 0;
    font-size: 12px;
    font-weight: 760;
    color: #2a4462;
  }

  .workspace-debug-layout {
    display: grid;
    grid-template-columns: minmax(0, 1.7fr) minmax(320px, 0.9fr);
    gap: 12px;
    align-items: start;
  }

  .workspace-debug-canvas-card,
  .workspace-debug-insight-card,
  .workspace-debug-item,
  .workspace-debug-disclosure {
    border: 1px solid rgba(205, 221, 240, 0.96);
    border-radius: 10px;
    background: rgba(255, 255, 255, 0.92);
  }

  .workspace-debug-canvas-card {
    padding: 12px;
  }

  .workspace-debug-canvas-card__title,
  .workspace-debug-insight-card__label {
    font-size: 11px;
    color: #64809f;
  }

  .workspace-debug-canvas-card__subtitle {
    margin-top: 4px;
    font-size: 16px;
    font-weight: 760;
    color: #244566;
  }

  .workspace-debug-canvas {
    margin-top: 10px;
    width: 100%;
    height: auto;
    aspect-ratio: 16 / 9;
    border: 1px solid rgba(201, 223, 245, 0.92);
    border-radius: 14px;
    background:
      radial-gradient(circle at top, rgba(245, 251, 255, 0.94), rgba(237, 246, 255, 0.98)),
      linear-gradient(180deg, rgba(255, 255, 255, 0.94), rgba(244, 249, 255, 0.96));
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.82);
    display: block;
  }

  .workspace-debug-canvas--empty {
    display: flex;
    align-items: center;
    justify-content: center;
    color: #8aa3be;
    font-size: 14px;
  }

  .workspace-debug-canvas__grid {
    fill: none;
    stroke: rgba(182, 205, 232, 0.42);
    stroke-width: 2;
    stroke-dasharray: 12 12;
  }

  .workspace-debug-canvas__path {
    fill: none;
    stroke: #2f7ed8;
    stroke-width: 14;
    stroke-linecap: round;
    stroke-linejoin: round;
    filter: drop-shadow(0 8px 20px rgba(47, 126, 216, 0.16));
  }

  .workspace-debug-canvas__start {
    fill: #eef6ff;
    stroke: #2f7ed8;
    stroke-width: 8;
  }

  .workspace-debug-canvas__arrow {
    fill: #eef6ff;
    stroke: #2f7ed8;
    stroke-width: 9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .workspace-debug-sidebar {
    display: grid;
    gap: 10px;
  }

  .workspace-debug-insight-grid {
    display: grid;
    gap: 10px;
  }

  .workspace-debug-insight-card {
    padding: 10px;
  }

  .workspace-debug-insight-card__value {
    display: block;
    margin-top: 6px;
    font-size: 14px;
    font-weight: 760;
    color: #244566;
    line-height: 1.4;
    word-break: break-word;
  }

  .workspace-debug-mini-preview {
    margin-top: 8px;
    display: grid;
    gap: 6px;
  }

  .workspace-debug-mini-preview strong {
    min-width: 0;
    font-size: 12px;
    color: #244566;
    font-weight: 700;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-debug-mini-preview__svg {
    width: 100%;
    max-width: 168px;
    height: 88px;
    border: 1px solid #c9dff5;
    border-radius: 10px;
    background: rgba(245, 251, 255, 0.95);
  }

  .workspace-debug-mini-preview__empty {
    height: 88px;
    border: 1px dashed rgba(201, 223, 245, 0.92);
    border-radius: 10px;
    background: rgba(245, 251, 255, 0.9);
    display: flex;
    align-items: center;
    justify-content: center;
    color: #8aa3be;
    font-size: 12px;
  }

  .workspace-debug-chip-row {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
  }

  .workspace-debug-chip-row--status {
    margin-top: -2px;
  }

  .workspace-debug-chip {
    border: 1px solid rgba(191, 211, 235, 0.92);
    border-radius: 999px;
    padding: 4px 9px;
    font-size: 11px;
    color: #355877;
    background: rgba(247, 251, 255, 0.96);
    white-space: nowrap;
  }

  .workspace-debug-chip--success {
    color: #1f6d4c;
    border-color: rgba(154, 214, 182, 0.92);
    background: rgba(235, 250, 241, 0.96);
  }

  .workspace-debug-chip--danger {
    color: #8f3f3f;
    border-color: rgba(235, 186, 186, 0.92);
    background: rgba(255, 241, 241, 0.96);
  }

  .workspace-debug-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 6px;
    margin-top: 8px;
  }

  .workspace-debug-item {
    padding: 6px 8px;
    display: grid;
    grid-template-columns: minmax(0, 1fr);
    gap: 8px;
  }

  .workspace-debug-item--wide {
    grid-column: 1 / -1;
  }

  .workspace-debug-item span {
    min-width: 0;
    font-size: 11px;
    color: #64809f;
  }

  .workspace-debug-item strong {
    min-width: 0;
    font-size: 12px;
    color: #244566;
    font-weight: 700;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-debug-gesture__grid {
    fill: none;
    stroke: rgba(157, 186, 219, 0.45);
    stroke-width: 1;
    stroke-dasharray: 2 2;
  }

  .workspace-debug-gesture__path {
    fill: none;
    stroke: #2f7ed8;
    stroke-width: 2.1;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .workspace-debug-gesture__start {
    fill: #ebf4ff;
    stroke: #2f7ed8;
    stroke-width: 1.5;
  }

  .workspace-debug-gesture__arrow {
    fill: none;
    stroke: #2f7ed8;
    stroke-width: 2.2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .workspace-debug-disclosure {
    overflow: hidden;
  }

  .workspace-debug-disclosure summary {
    list-style: none;
    cursor: pointer;
    padding: 10px 12px;
    font-size: 12px;
    font-weight: 700;
    color: #32567a;
    user-select: none;
  }

  .workspace-debug-disclosure summary::-webkit-details-marker {
    display: none;
  }

  .workspace-debug-disclosure[open] summary {
    border-bottom: 1px solid rgba(195, 213, 232, 0.88);
  }

  .workspace-debug-events__list {
    display: flex;
    flex-direction: column;
    gap: 5px;
    max-height: 192px;
    overflow: auto;
    padding: 10px;
  }

  .workspace-debug-event-row {
    display: grid;
    grid-template-columns: auto 1fr auto auto auto;
    gap: 6px;
    align-items: center;
    border: 1px solid rgba(204, 221, 241, 0.92);
    border-radius: 8px;
    padding: 5px 6px;
    background: rgba(255, 255, 255, 0.9);
  }

  .workspace-debug-event-row__seq {
    font-size: 11px;
    color: #37587f;
    font-weight: 700;
    min-width: 34px;
  }

  .workspace-debug-event-row__line {
    min-width: 0;
    font-size: 11px;
    color: #335170;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-debug-event-row__tag {
    font-size: 10px;
    color: #3f5f82;
    border: 1px solid rgba(191, 211, 235, 0.9);
    border-radius: 999px;
    padding: 2px 6px;
    background: rgba(238, 246, 255, 0.9);
    white-space: nowrap;
  }

  .workspace-debug-action-steps {
    display: grid;
    gap: 4px;
    margin: -2px 10px 10px 32px;
  }

  .workspace-debug-action-step {
    display: grid;
    grid-template-columns: auto 1fr auto;
    gap: 6px;
    align-items: center;
    font-size: 11px;
    color: #35506c;
  }

  .workspace-debug-action-step__index {
    color: #6a86a5;
    font-weight: 700;
  }

  .workspace-debug-action-step__line {
    min-width: 0;
  }

  .workspace-debug-action-step__detail {
    color: #6a86a5;
    white-space: nowrap;
  }

  @media (max-width: 1080px) {
    .workspace-debug-layout {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 860px) {
    .workspace-debug-grid {
      grid-template-columns: 1fr;
    }
  }
</style>
