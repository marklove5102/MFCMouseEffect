# Phase 53v - WebSettings Automation Runtime Routes Layer Split

## Background
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationRoutes.cpp` mixed three concerns:
  - shortcut-capture session lifecycle routes
  - active-process/app-catalog routes
  - shared JSON parsing and app-catalog cache utilities
- This increased coupling for automation route evolution and contract review.

## Decision
- Keep runtime automation API contracts unchanged.
- Split by responsibility:
  - `shortcut-capture` route layer
  - `process/catalog` route layer
  - shared route utility layer
- Keep `WebSettingsServer.AutomationRoutes.cpp` as thin delegating entry.

## Code Changes
1. Added shortcut-capture route layer
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationShortcutCaptureRoutes.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationShortcutCaptureRoutes.cpp`
- Owns:
  - `POST /api/automation/shortcut-capture/start`
  - `POST /api/automation/shortcut-capture/poll`
  - `POST /api/automation/shortcut-capture/stop`

2. Added process/catalog route layer
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationCatalogRoutes.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationCatalogRoutes.cpp`
- Owns:
  - `POST /api/automation/active-process`
  - `POST /api/automation/app-catalog`

3. Added shared utility layer
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationRouteUtils.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationRouteUtils.cpp`
- Shared:
  - JSON response helper
  - object payload parse helper
  - shortcut-capture session/state text helpers
  - app-catalog `force` parse + 30s TTL cache loading

4. Delegator and build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationRoutes.cpp`
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`

## Behavior Compatibility
- API paths, payload schema, and response fields unchanged.
- Runtime app-catalog cache semantics unchanged (`30s` TTL + `force` bypass).
- This phase is structure-only refactor.

## Functional Ownership
- Category: `手势映射`
- Coverage: runtime automation control-plane routes (`shortcut capture`, `active process`, `app catalog`).

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.
