# Phase 53n - WebSettings Request Gateway Module Split

## Background
- After phase53m, `WebSettingsServer.Routing.cpp` still handled request-entry concerns:
  - API token gate
  - query-string stripping
  - favicon/404 path fallback
  - exception-to-response conversion
- This mixed request gateway concerns with route delegation concerns.

## Decision
- Keep behavior unchanged.
- Extract entry gateway logic to a dedicated module:
  - `WebSettingsServer.RequestGateway.h`
  - `WebSettingsServer.RequestGateway.cpp`
- Keep `Routing.cpp` as top-level route delegator:
  - `core` / `automation` / `test` / `wasm` / `static asset`

## Code Changes
1. New request gateway module
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.RequestGateway.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.RequestGateway.cpp`
- Moved and centralized:
  - API token validation branch
  - query-string normalization
  - favicon no-content handling
  - static asset fallback + 404 handling
  - exception-to-HTTP error mapping (`API -> JSON 500`, `non-API -> text 500`)

2. Routing file simplified
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- `HandleRequest` now calls `HandleWebSettingsRequestGateway(...)` with callbacks:
  - `IsTokenValid`
  - `HandleApiRoute`
  - `HandleStaticAssetRoute`

3. Build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added `WebSettingsServer.RequestGateway.cpp` to runtime source list.

## Behavior Compatibility
- Request authorization semantics unchanged.
- API/static/fallback behavior unchanged.
- Error response format unchanged.
- This phase is a structural refactor only.

## Verification
1. `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- Result: passed.

2. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

## Risks
- Risk: callback boundary miswire could affect request dispatch.
- Mitigation: full suite covers scaffold/core smoke, API contracts, macOS selfchecks, Linux compile gate, and WebUI automation semantics.
