# Phase 53q - WebSettings Test Routes Deep Split

## Background
- `WebSettingsServer.TestApiRoutes.cpp` remained a large mixed file with:
  - automation test probes
  - input-indicator test probe
  - wasm dispatch test probe
  - shared JSON/env parsing helpers
- This increased coupling and change risk for test-contract extensions.

## Decision
- Keep test API behavior and route paths unchanged.
- Split test routes by responsibility:
  - shared test helper layer
  - automation test route layer
  - wasm/input-indicator test route layer
- Keep `WebSettingsServer.TestApiRoutes.cpp` as thin delegating entry.

## Code Changes
1. Shared helper layer
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestRouteCommon.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestRouteCommon.cpp`
- Provides:
  - JSON/plain response helpers
  - JSON payload parsing
  - env-flag parsing
  - common scalar parser helpers (`bool/int32/button`)

2. Automation test route layer
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestAutomationApiRoutes.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestAutomationApiRoutes.cpp`
- Handles existing automation test endpoints unchanged:
  - `/api/automation/test-app-scope-match`
  - `/api/automation/test-binding-priority`
  - `/api/automation/test-match-and-inject`
  - `/api/automation/test-shortcut-from-mac-keycode`
  - `/api/automation/test-inject-shortcut`

3. WASM/input-indicator test route layer
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestWasmInputApiRoutes.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestWasmInputApiRoutes.cpp`
- Handles existing endpoints unchanged:
  - `/api/input-indicator/test-mouse-labels`
  - `/api/wasm/test-dispatch-click`

4. Test route entry simplification
- Replaced `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestApiRoutes.cpp` with delegating entry only.

5. Build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added new split source files to runtime common source list.

## Behavior Compatibility
- API paths, env gates, payload schema, and return structures are unchanged.
- This phase is structure-only for test-contract implementation.

## Functional Ownership
- Category: `测试契约层`
- Coverage:
  - `手势映射` (automation probe contracts)
  - `键鼠指示` (indicator probe contracts)
  - `WASM` (dispatch probe contracts)

## Verification
1. `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- Result: passed.

2. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.
