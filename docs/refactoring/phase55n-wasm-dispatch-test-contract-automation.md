# Phase 55n: WASM Dispatch Test Contract Automation

## Issue Classification
- Verdict: `Evidence gap`.
- Symptom: core HTTP contracts covered WASM state/capabilities/catalog, but not "plugin loaded -> invoke -> render" end-to-end behavior in automated checks.
- Impact: manual verification cost remained high for macOS WASM path.

## Goal
1. Add a test-friendly WASM dispatch endpoint for non-interactive regression.
2. Keep production behavior unchanged by gating endpoint behind explicit env flag.
3. Extend core automation contract script to validate invoke/render path automatically.

## Implementation
- Added test-only endpoint:
  - `POST /api/wasm/test-dispatch-click`
  - file: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
  - behavior:
    - gated by env: `MFX_ENABLE_WASM_TEST_DISPATCH_API`
    - disabled state returns `404 not found`
    - enabled state invokes wasm click dispatch via `WasmEventInvokeExecutor`
    - returns structured fields (`route_active`, `invoke_ok`, `rendered_any`, command/throttle counters, runtime diagnostics)
- Contract script extension:
  - file: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`
  - startup now enables test endpoint in regression process:
    - `MFX_ENABLE_WASM_TEST_DISPATCH_API=1` (override-able)
  - test flow:
    1. call `/api/wasm/catalog`
    2. resolve manifest (prefer `examples/wasm-plugin-template/dist/plugin.json`)
    3. call `/api/wasm/import-selected`
    4. call `/api/wasm/load-manifest`
    5. call `/api/wasm/enable`
    6. call `/api/wasm/test-dispatch-click`
    7. assert `route_active=true` and `invoke_ok=true`; on macOS additionally assert `rendered_any=true`
- Also kept non-interactive folder-dialog probe check:
  - `POST /api/wasm/import-from-folder-dialog` with `{"probe_only":true}`

## Behavior Change
- No default user-visible behavior change (endpoint is off unless env enabled).
- Regression workflow now has machine-verifiable invoke/render evidence on macOS core lane.

## Validation
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto` (pass)

## Next
- Continue Phase 55o for manual UX acceptance only (dialog interaction + plugin switching ergonomics), while keeping automated contracts as baseline gate.
