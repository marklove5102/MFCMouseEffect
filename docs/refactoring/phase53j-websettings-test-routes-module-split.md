# Phase 53j: WebSettings Test Routes Module Split

## Issue Classification
- Verdict: `Architecture debt`.
- Problem: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp` exceeded 1k lines and mixed production routes with test-only contracts, increasing coupling and review risk.

## Goal
1. Split test-only API routing into a dedicated module.
2. Keep public API behavior and contracts unchanged.
3. Reduce main routing file size and isolate test-gate evolution from production route flow.

## Implementation
1. Added dedicated test-route module
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestApiRoutes.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestApiRoutes.cpp`
- Exposed one entry:
  - `HandleWebSettingsTestApiRoute(req, path, controller, resp)`
- Migrated test-only endpoints:
  - `/api/automation/test-app-scope-match`
  - `/api/automation/test-binding-priority`
  - `/api/automation/test-match-and-inject`
  - `/api/automation/test-shortcut-from-mac-keycode`
  - `/api/automation/test-inject-shortcut`
  - `/api/input-indicator/test-mouse-labels`
  - `/api/wasm/test-dispatch-click`

2. Main route file now delegates
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- Added delegation call:
  - `if (HandleWebSettingsTestApiRoute(...)) return true;`
- Removed migrated helper blocks from main file.

3. Build wiring
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added:
  - `MouseFx/Server/WebSettingsServer.TestApiRoutes.cpp`

## Validation
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Closure
- Main routing file responsibility is narrower and easier to maintain.
- Test contract surface is isolated, with no behavioral regression in existing suite gates.
