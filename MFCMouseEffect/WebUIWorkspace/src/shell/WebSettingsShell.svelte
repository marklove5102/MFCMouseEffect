<script>
  import TopBar from "./TopBar.svelte";
  import SettingsGrid from "./SettingsGrid.svelte";

  export let statusMessage = "";
  export let statusTone = "";
  export let actionsDisabled = false;
  export let onAction = () => {};

  function handleAction(event) {
    const type = event?.detail?.type;
    if (!type) return;
    onAction(type);
  }
</script>

<div class="page-bg" aria-hidden="true">
  <div class="shape shape-a"></div>
  <div class="shape shape-b"></div>
  <div class="shape shape-c"></div>
</div>

<div class="wrap">
  <TopBar
    {actionsDisabled}
    {statusMessage}
    {statusTone}
    on:action={handleAction}
  />

  <section class="workspace-frame">
    <div class="workspace-topnav-shell">
      <div class="workspace-topnav" id="workspace_sidebar_mount"></div>
    </div>

    <main class="workspace-main workspace-main--full">
      <div id="workspace_hint_mount"></div>
      <div id="workspace_aux_mount"></div>
      <SettingsGrid />
    </main>
  </section>
</div>
