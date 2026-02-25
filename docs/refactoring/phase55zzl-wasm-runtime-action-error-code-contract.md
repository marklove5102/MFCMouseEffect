# Phase 55zzl: WASM Runtime Action Error-Code Contract

## Capability
- WASM

## Why
- Runtime action routes (`/api/wasm/load-manifest`, `/api/wasm/reload`) previously returned only free-form `error` text.
- Transfer routes already use structured `error_code`; runtime action routes had inconsistent error semantics.
- Missing `manifest_path` for `load-manifest` had no explicit regression/manual contract.

## Scope
- Add structured `error_code` to runtime action responses while preserving existing fields.
- Normalize wasm route path parsing by trimming surrounding ASCII whitespace.
- Extend regression/manual checks with:
  - `reload` success contract.
  - `load-manifest` missing-path negative contract.

## Code Changes

### 1) Runtime action response model hardening
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRouteUtils.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRouteResponseUtils.cpp`
- Behavior:
  - success path now returns `error_code=""`.
  - failure path now resolves `error_code` from:
    1. route-provided default code,
    2. wasm diagnostics (`lastLoadFailureCode`),
    3. fallback `unknown_error`.

### 2) `load-manifest` / `reload` route codes
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmLoadManifestRoute.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmReloadRoute.cpp`
- Added deterministic route-level codes:
  - `no_controller`
  - `wasm_host_unavailable`
  - `manifest_path_required`
  - `load_manifest_failed`
  - `reload_failed`
  - `reload_target_missing`
- Existing diagnostics fields (`last_load_failure_stage/code`) remain unchanged and continue to carry detailed load failures.

### 3) Path parse normalization
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRouteParseUtils.cpp`
- Behavior:
  - `manifest_path` and `initial_path` are now trimmed (`TrimAscii`) before downstream handling.

### 4) Contract coverage expansion
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_contract_checks.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/lib/wasm_selfcheck_common.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`
- Added/expanded checks:
  - `load-manifest ok -> error_code=""`.
  - `reload ok -> error_code=""`.
  - `load-manifest` missing payload path -> `error_code="manifest_path_required"`.
  - `load-manifest` invalid path failure now asserts both diagnostics fields and `error_code`.

## Validation
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_contract_checks.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/lib/wasm_selfcheck_common.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`
- `./tools/platform/regression/run-posix-core-wasm-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- Backward-compatible additive change:
  - `load-manifest` / `reload` responses now include `error_code`.
- Error handling is now script-guarded with deterministic negative-path assertions, reducing runtime/API drift risk.
