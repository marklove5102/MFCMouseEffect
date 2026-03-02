# Phase 47: macOS Tray Menu Localization

## Top Decisions (Important)
- Keep tray host/service lifecycle code separate from text localization policy.
- Localize tray menu labels by macOS preferred language (`NSLocale preferredLanguages`), without changing shell API contracts.
- Preserve cross-host build by using `.mm` (Apple) and `.cpp` (fallback) split for localization provider.

## Changes
1. Added localization module:
   - `Platform/macos/Shell/MacosTrayMenuLocalization.h`
   - `Platform/macos/Shell/MacosTrayMenuLocalization.mm` (Apple host implementation)
   - `Platform/macos/Shell/MacosTrayMenuLocalization.cpp` (non-Apple fallback)
2. Updated tray service:
   - `Platform/macos/Shell/MacosTrayService.mm` now resolves menu text through localization module.
   - keeps default English fallback if UTF-8 conversion fails.
3. Updated package build wiring:
   - `Platform/macos/CMakeLists.txt`
   - selects `.mm` localization source on Apple host, `.cpp` on cross-host scaffolding build.

## Behavior
- When preferred language starts with `zh`, tray labels become:
  - settings: `设置`
  - exit: `退出`
- Other locales keep English defaults:
  - `Settings`
  - `Exit`

## Validation
- Executed:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
- Result:
  - build passed
  - smoke checks passed
  - scaffold HTTP checks passed
