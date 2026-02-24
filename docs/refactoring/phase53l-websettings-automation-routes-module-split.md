# Phase 53l - WebSettings Automation Routes Module Split

## Background
- After phase53k, `WebSettingsServer.Routing.cpp` still contained all production `/api/automation/*` route logic.
- The main routing file remained responsible for both shared settings endpoints and automation workflow internals.

## Decision
- Keep behavior/API contracts unchanged.
- Split production automation routes into a dedicated module:
  - `WebSettingsServer.AutomationRoutes.h`
  - `WebSettingsServer.AutomationRoutes.cpp`
- Main routing file now delegates automation path matching via:
  - `HandleWebSettingsAutomationApiRoute(req, path, controller_, resp)`

## Code Changes
1. New automation route module
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationRoutes.cpp`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.AutomationRoutes.h`
- Moved automation route handlers:
  - `POST /api/automation/shortcut-capture/start`
  - `POST /api/automation/shortcut-capture/poll`
  - `POST /api/automation/shortcut-capture/stop`
  - `POST /api/automation/active-process`
  - `POST /api/automation/app-catalog`

2. Main route file shrink
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- Removed automation-specific helper functions and inline route branches.
- Added include and delegation call to automation route module.

3. Build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added `WebSettingsServer.AutomationRoutes.cpp` to runtime common source list.

## Behavior Compatibility
- Endpoint paths, request payloads, and response schema remain unchanged.
- Shortcut capture session, foreground process query, and app catalog cache semantics remain unchanged.
- This phase is structure-only and should not change runtime behavior.

## Verification
1. `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- Result: passed.

2. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

## Risks
- Risk: route-order or payload parsing drift after extraction.
- Mitigation: automation routes are guarded by core automation HTTP contract regression and the full POSIX regression suite.
