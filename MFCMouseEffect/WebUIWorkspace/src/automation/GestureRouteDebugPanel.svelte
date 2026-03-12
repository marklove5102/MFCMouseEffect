<script>
  import {
    boolLabel,
    formatGestureRouteModifiers,
    gestureRouteSourceLabel,
    normalizeGestureRouteStatus,
  } from './gesture-route-debug-model.js';

  export let payloadState = {};
  export let platform = 'windows';
  export let texts = {};

  let routeStatus = null;

  function t(key, fallback) {
    const value = `${texts?.[key] || ''}`.trim();
    return value || fallback;
  }

  $: routeStatus = normalizeGestureRouteStatus(payloadState);
</script>

{#if routeStatus}
  <div class="automation-gesture-debug">
    <div class="automation-gesture-debug-title">{t('title', 'Gesture Realtime Debug')}</div>
    <div class="automation-gesture-debug-grid">
      <div class="automation-gesture-debug-item">
        <span>{t('lastStage', 'Stage')}</span>
        <strong>{routeStatus.lastStage || '-'}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('lastReason', 'Reason')}</span>
        <strong>{routeStatus.lastReason || '-'}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('lastGesture', 'Gesture')}</span>
        <strong>{routeStatus.lastGestureId || '-'}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('triggerButton', 'Trigger Button')}</span>
        <strong>{routeStatus.lastTriggerButton || '-'}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('matched', 'Matched')}</span>
        <strong>{boolLabel(routeStatus.lastMatched, texts)}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('injected', 'Injected')}</span>
        <strong>{boolLabel(routeStatus.lastInjected, texts)}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('source', 'Source')}</span>
        <strong>{gestureRouteSourceLabel(routeStatus, texts)}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('samples', 'Samples')}</span>
        <strong>{routeStatus.lastSamplePointCount}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('modifiers', 'Modifiers')}</span>
        <strong>{formatGestureRouteModifiers(routeStatus.modifiers, platform, texts)}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('mappings', 'Mappings')}</span>
        <strong>{routeStatus.gestureMappingCount}</strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('buttonless', 'Buttonless')}</span>
        <strong>
          {boolLabel(routeStatus.buttonlessGestureEnabled, texts)}
          ({routeStatus.buttonlessGestureMappingCount})
        </strong>
      </div>
      <div class="automation-gesture-debug-item">
        <span>{t('pointerDown', 'Pointer Down')}</span>
        <strong>{boolLabel(routeStatus.pointerButtonDown, texts)}</strong>
      </div>
    </div>
  </div>
{/if}

<style>
  .automation-gesture-debug {
    border: 1px solid #c9dbef;
    border-radius: 10px;
    background: rgba(243, 249, 255, 0.96);
    padding: 8px 10px;
  }

  .automation-gesture-debug-title {
    font-size: 12px;
    font-weight: 700;
    color: #2d4f70;
    margin-bottom: 6px;
  }

  .automation-gesture-debug-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 6px 8px;
  }

  .automation-gesture-debug-item {
    min-width: 0;
    border: 1px solid #d6e4f2;
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.9);
    padding: 6px 8px;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .automation-gesture-debug-item span {
    font-size: 11px;
    line-height: 1.2;
    color: #64809f;
  }

  .automation-gesture-debug-item strong {
    font-size: 12px;
    line-height: 1.25;
    color: #234766;
    font-weight: 700;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  @media (max-width: 1080px) {
    .automation-gesture-debug-grid {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 760px) {
    .automation-gesture-debug-grid {
      grid-template-columns: 1fr;
    }
  }
</style>
