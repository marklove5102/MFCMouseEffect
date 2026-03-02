# Phase 49: macOS Tray Menu Standard Shortcuts

## Top Decisions (Important)
- Keep tray menu item labels/localization logic unchanged; only add keyboard shortcuts.
- Use native macOS conventions for tray menu actions:
  - `Settings`: `Cmd+,`
  - `Exit`: `Cmd+Q`
- Keep changes isolated to macOS tray implementation to avoid Windows/Linux regressions.

## Changes
1. Updated:
   - `Platform/macos/Shell/MacosTrayService.mm`
2. Tray menu item wiring:
   - settings item key equivalent changed from empty to `","`
   - exit item key equivalent changed from empty to `"q"`
   - both items now set modifier mask `NSEventModifierFlagCommand`

## Behavior
- Tray menu now provides standard macOS keyboard shortcuts for settings and quit actions.
- Existing click behavior and localized labels remain unchanged.

## Validation
- Executed:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
- Result:
  - build passed
  - smoke checks passed
  - scaffold HTTP checks passed
