<script>
  import AutomationSidebarDebugCard from './AutomationSidebarDebugCard.svelte';

  export let activeSectionId = '';
  export let runtimePlatform = 'windows';
  export let runtimeState = {};
  export let i18n = {};
</script>

{#if activeSectionId === 'automation'}
  <AutomationSidebarDebugCard
    payloadState={runtimeState}
    platform={runtimePlatform}
    {i18n}
  />
{/if}
