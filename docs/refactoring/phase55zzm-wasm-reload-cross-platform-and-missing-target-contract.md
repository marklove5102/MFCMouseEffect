# Phase 55zzm: WASM Reload Cross-Platform Fix and Missing-Target Contract

## Capability
- WASM

## Why
- `wasm_reload` command path in `CommandHandler` was still Windows-gated, causing POSIX lane reload to degenerate into no-op behavior.
- Reload negative path (`no active plugin target`) lacked deterministic regression/manual coverage.

## Scope
- Fix reload command to execute on macOS/Linux core lane.
- Keep Windows behavior unchanged (same host API path, now unified).
- Add deterministic test probe to reset runtime plugin target and verify reload failure code semantics.

## Code Changes

### 1) Root fix: reload command now truly cross-platform
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/CommandHandler.cpp`
- Change:
  - removed `MFX_PLATFORM_WINDOWS` guard around `HandleWasmReloadCommand`.
  - now invokes `WasmHost()->ReloadPlugin()` on all platforms where host exists.

### 2) Reload error-code precedence hardening
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmReloadRoute.cpp`
- Change:
  - when reload starts without active target, response now deterministically returns:
    - `ok=false`
    - `error_code="reload_target_missing"`
  - this no longer depends on stale `lastLoadFailureCode`.

### 3) Deterministic runtime-reset probe for contract testing
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestWasmInputApiRoutes.cpp`
- Added test-gated endpoint:
  - `POST /api/wasm/test-reset-runtime`
- Behavior:
  - unloads current WASM plugin target in host runtime.
  - returns runtime target-cleared flags for script assertions.

### 4) Regression and manual selfcheck expansion
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_contract_checks.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/lib/wasm_selfcheck_common.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`
- New assertions:
  - reset runtime probe passes and clears active target fields.
  - reload after reset returns `error_code="reload_target_missing"`.
  - reload/restore flow remains recoverable via `load-manifest`.

## Validation
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_helpers.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_wasm_contract_checks.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/lib/wasm_selfcheck_common.sh`
- `bash -n /Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`
- `./tools/platform/regression/run-posix-core-wasm-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- Reload behavior now aligns across Windows/macOS/Linux core lane.
- Missing-target reload failure is now deterministic and script-guarded, reducing hidden no-op risk in WASM runtime operations.
