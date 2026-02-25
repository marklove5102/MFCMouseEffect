# Phase 53x - WebSettings WASM Runtime Routes Layer Split

## Background
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRuntimeRoutes.cpp` mixed two concerns:
  - runtime state/policy routes (`enable/disable/policy`)
  - runtime action routes (`reload/load-manifest`)
- This increased coupling for runtime API evolution and review.

## Decision
- Keep existing runtime API paths and response contracts unchanged.
- Split runtime routes by responsibility:
  - state/policy route layer
  - action route layer
- Keep `WebSettingsServer.WasmRuntimeRoutes.cpp` as thin delegating entry.

## Code Changes
1. Added state/policy route layer
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRuntimeStateRoutes.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRuntimeStateRoutes.cpp`
- Owns:
  - `POST /api/wasm/enable`
  - `POST /api/wasm/policy`
  - `POST /api/wasm/disable`

2. Added action route layer
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRuntimeActionRoutes.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRuntimeActionRoutes.cpp`
- Owns:
  - `POST /api/wasm/reload`
  - `POST /api/wasm/load-manifest`

3. Delegator and build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRuntimeRoutes.cpp`
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`

## Behavior Compatibility
- Endpoint paths and payload/response fields unchanged.
- Runtime diagnostics and error mapping unchanged.
- This phase is structure-only refactor.

## Functional Ownership
- Category: `WASM`
- Coverage: runtime control-plane routes (`state/policy` + `reload/load-manifest`).

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed (executed with sandbox-escalated run to allow full local socket/probe checks).
