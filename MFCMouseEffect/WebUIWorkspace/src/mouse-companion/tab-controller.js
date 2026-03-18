export function createMouseCompanionTabController(options) {
  const opts = options && typeof options === 'object' ? options : {};
  const byId = typeof opts.byId === 'function' ? opts.byId : () => null;
  const tabIds = Array.isArray(opts.tabIds) ? opts.tabIds.filter((id) => `${id || ''}`.trim()) : [];
  const defaultTabId =
    tabIds.includes(`${opts.defaultTabId || ''}`.trim()) ? `${opts.defaultTabId}`.trim() : tabIds[0] || '';
  const onActiveTabChange =
    typeof opts.onActiveTabChange === 'function' ? opts.onActiveTabChange : () => {};

  function normalizeTabId(input) {
    const value = `${input || ''}`.trim().toLowerCase();
    if (tabIds.includes(value)) {
      return value;
    }
    return defaultTabId;
  }

  let latestActiveTab = normalizeTabId(opts.initialActiveTab);

  function syncTabUi() {
    for (const tabId of tabIds) {
      const button = byId(`mc_tab_${tabId}`);
      if (button) {
        const active = tabId === latestActiveTab;
        button.classList.toggle('is-active', active);
        button.setAttribute('aria-selected', active ? 'true' : 'false');
        button.setAttribute('tabindex', active ? '0' : '-1');
      }
      const panel = byId(`mc_panel_${tabId}`);
      if (panel) {
        panel.hidden = tabId !== latestActiveTab;
      }
    }
  }

  function setActiveTab(tabId, options = {}) {
    const localOptions = options && typeof options === 'object' ? options : {};
    const persist = localOptions.persist !== false;
    const normalized = normalizeTabId(tabId);
    latestActiveTab = normalized;
    if (persist) {
      onActiveTabChange(normalized);
    }
    syncTabUi();
  }

  function bindTabActions() {
    for (const tabId of tabIds) {
      const node = byId(`mc_tab_${tabId}`);
      if (!node) {
        continue;
      }
      node.addEventListener('click', () => {
        setActiveTab(tabId);
      });
      node.addEventListener('keydown', (event) => {
        const code = `${event.key || ''}`.toLowerCase();
        if (code !== 'arrowleft' && code !== 'arrowright') {
          return;
        }
        event.preventDefault();
        const currentIndex = tabIds.indexOf(latestActiveTab);
        const baseIndex = currentIndex >= 0 ? currentIndex : 0;
        const delta = code === 'arrowright' ? 1 : -1;
        const nextIndex = (baseIndex + delta + tabIds.length) % tabIds.length;
        const nextTabId = tabIds[nextIndex];
        setActiveTab(nextTabId);
        byId(`mc_tab_${nextTabId}`)?.focus();
      });
    }
  }

  return {
    getActiveTab: () => latestActiveTab,
    setActiveTab,
    syncTabUi,
    bindTabActions,
  };
}
