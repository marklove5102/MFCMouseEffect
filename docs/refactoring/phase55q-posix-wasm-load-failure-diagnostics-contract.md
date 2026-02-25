# Phase 55q: POSIX WASM Load-Failure Diagnostics Contract

## Issue Classification
- Verdict: `Bug/regression`.
- Evidence: invalid-manifest selfcheck expected machine-readable `manifest_io_error`, but missing-manifest message classification was not consistently mapped, causing contract assertion drift.

## Goal
1. Expose stable, machine-readable WASM load-failure diagnostics in both state and route responses.
2. Normalize manifest load-failure classification for script-level assertions and UI diagnostics.
3. Keep existing Windows behavior and POSIX runtime flow unchanged except diagnostics enrichment.

## Implementation
- WASM host diagnostics enrichment:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmEffectHost.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmEffectHost.cpp`
  - Added `lastLoadFailureStage` and `lastLoadFailureCode`.
  - Introduced `SetLoadFailure(...)` / `ClearLoadFailure()` to unify failure bookkeeping.
  - Manifest load errors now classify into explicit codes:
    - `manifest_io_error`
    - `manifest_json_parse_error`
    - `manifest_invalid`
- WebSettings state/route contract extension:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.Diagnostics.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmRouteResponseUtils.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestWasmInputApiRoutes.cpp`
  - Added response/state fields:
    - `last_load_failure_stage`
    - `last_load_failure_code`
- Schema synchronization:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.CapabilitiesSections.cpp`
  - Added both new diagnostics keys to schema diagnostics list.
- Selfcheck contract closure:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`
  - Invalid-manifest step now asserts `stage=manifest_load` and `code=manifest_io_error`.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Closure
- WASM load-failure diagnostics now provide stable stage/code pairs for automation scripts, WebSettings diagnostics panels, and future triage tooling without changing core runtime success-path behavior.
