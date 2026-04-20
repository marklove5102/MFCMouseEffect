(() => {
  const catalog = {
    mouse: [
      {
        id: 'browser_tabs',
        i18nKey: 'auto_template_mouse_tabs',
        fallback: 'Browser tabs (wheel switch, middle close)',
        bindings: [
          { enabled: true, trigger: 'scroll_up', shortcut: 'Ctrl+Shift+Tab' },
          { enabled: true, trigger: 'scroll_down', shortcut: 'Ctrl+Tab' },
          { enabled: true, trigger: 'middle_click', shortcut: 'Ctrl+W' },
        ],
      },
      {
        id: 'document_pages',
        i18nKey: 'auto_template_mouse_pages',
        fallback: 'Document paging (PageUp/PageDown)',
        bindings: [
          { enabled: true, trigger: 'scroll_up', shortcut: 'PageUp' },
          { enabled: true, trigger: 'scroll_down', shortcut: 'PageDown' },
        ],
      },
    ],
    gesture: [
      {
        id: 'window_snap',
        i18nKey: 'auto_template_gesture_window',
        fallback: 'Window snap (Win+Arrow)',
        platforms: ['windows'],
        bindings: [
          { enabled: true, trigger: 'left', shortcut: 'Win+Left' },
          { enabled: true, trigger: 'right', shortcut: 'Win+Right' },
          { enabled: true, trigger: 'up', shortcut: 'Win+Up' },
          { enabled: true, trigger: 'down', shortcut: 'Win+Down' },
        ],
      },
      {
        id: 'browser_navigation',
        i18nKey: 'auto_template_gesture_browser',
        fallback: 'Browser navigation (Alt+Left/Right)',
        bindings: [
          { enabled: true, trigger: 'left', shortcut: 'Alt+Left' },
          { enabled: true, trigger: 'right', shortcut: 'Alt+Right' },
          { enabled: true, trigger: 'up', shortcut: 'Ctrl+L' },
          { enabled: true, trigger: 'down', shortcut: 'Ctrl+R' },
        ],
      },
    ],
  };

  function normalizePlatform(value) {
    const text = `${value || ''}`.trim().toLowerCase();
    if (!text) return '';
    if (text === 'windows' || text.startsWith('win')) return 'windows';
    if (
      text === 'macos' ||
      text === 'mac' ||
      text === 'darwin' ||
      text === 'osx' ||
      text === 'macosx' ||
      text.includes('mac') ||
      text.includes('darwin')
    ) {
      return 'macos';
    }
    if (text === 'linux' || text.includes('linux') || text.includes('x11')) return 'linux';
    return '';
  }

  function supportsPlatform(item, platform) {
    const target = normalizePlatform(platform);
    const platforms = Array.isArray(item?.platforms) ? item.platforms : [];
    if (!target || platforms.length === 0) {
      return true;
    }
    for (const entry of platforms) {
      if (normalizePlatform(entry) === target) {
        return true;
      }
    }
    return false;
  }

  function list(kind, translate, platform) {
    const t = typeof translate === 'function' ? translate : (key, fallback) => fallback || key || '';
    const items = catalog[kind] || [];
    return items
      .filter((item) => supportsPlatform(item, platform))
      .map((item) => ({
        id: item.id,
        label: t(item.i18nKey, item.fallback),
      }));
  }

  function mappings(kind, id) {
    const items = catalog[kind] || [];
    const hit = items.find((item) => item.id === id);
    if (!hit) return [];
    return hit.bindings.map((binding) => ({
      enabled: binding.enabled !== false,
      trigger: binding.trigger || '',
      actions: binding.shortcut ? [{ type: 'send_shortcut', shortcut: binding.shortcut }] : [],
    }));
  }

  window.MfxAutomationTemplates = {
    list,
    mappings,
  };
})();
