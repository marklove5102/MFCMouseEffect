# Phase 55l: POSIX WASM Catalog and Transfer APIs

## Issue Classification
- Verdict: `Bug/regression risk`.
- Symptom: macOS core lane had WASM runtime/render support, but `WebSettings` still returned platform-unsupported errors for plugin catalog/import/export APIs.
- Impact: plugin path operations in shared Svelte WebUI were inconsistent across platforms and blocked macOS first-line workflow.

## Goal
1. Enable WASM catalog/import/export HTTP APIs on POSIX core lane.
2. Keep Windows behavior unchanged.
3. Preserve Linux compile-level follow.

## Implementation
- Enabled cross-platform API routing for:
  - `POST /api/wasm/catalog`
  - `POST /api/wasm/import-selected`
  - `POST /api/wasm/export-all`
  - File: `MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- Added missing WASM plugin units into POSIX runtime build:
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginCatalog.cpp`
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginTransferService.cpp`
  - File: `MFCMouseEffect/Platform/CMakeLists.txt`
- Removed Win32-only time API dependency from transfer service:
  - replaced `SYSTEMTIME/GetLocalTime` with `std::chrono + localtime_s/localtime_r`.
  - file: `MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginTransferService.cpp`
- Fixed POSIX compile blocker in catalog scanner:
  - replaced `_wcsicmp` with local case-insensitive compare helper.
  - file: `MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginCatalog.cpp`
- Extended core HTTP contract regression:
  - assert `wasm catalog` endpoint is `200 + ok=true` and has expected fields.
  - on macOS, assert no `wasm_catalog_not_supported_on_this_platform` marker remains.
  - file: `tools/platform/regression/lib/core_http.sh`

## Behavior Change
- On macOS core lane:
  - `WASM catalog/import/export` APIs are now available through the same shared WebSettings backend path as Windows.
- `POST /api/wasm/import-from-folder-dialog` remains platform-gated (native picker not implemented on POSIX yet).

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto` (pass)

## Next
- Continue Phase 55 manual acceptance closure:
  - real plugin load/invoke/render verification from shared WebUI flow on macOS.
