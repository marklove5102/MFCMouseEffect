# Phase 55j: macOS WASM Throttle Cause Breakdown

## Goal
- Improve diagnostics precision by separating throttle causes.
- Keep runtime/UI contracts explicit so burst behavior can be triaged quickly.

## Implementation
- Extended macOS overlay render result taxonomy:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTransientOverlay.h`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTransientOverlay.mm`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.mm`
  - `Throttled` split to:
    - `ThrottledByCapacity`
    - `ThrottledByInterval`
- Extended command execution diagnostics:
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmCommandExecutionResult.h`
  - Added:
    - `throttledByCapacityCommands`
    - `throttledByIntervalCommands`
- Extended host diagnostics/state mapping:
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmEffectHost.h`
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmEffectHost.cpp`
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmEventInvokeExecutor.cpp`
  - `MFCMouseEffect/MouseFx/Server/SettingsStateMapper.cpp`
  - `MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
  - Added API fields:
    - `last_throttled_by_capacity_render_commands`
    - `last_throttled_by_interval_render_commands`
- Schema + regression contract update:
  - `MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.cpp`
  - `tools/platform/regression/lib/core_http.sh`
- WebUI diagnostics visibility:
  - `MFCMouseEffect/WebUIWorkspace/src/wasm/WasmPluginFields.svelte`
  - `MFCMouseEffect/WebUIWorkspace/src/entries/wasm-main.js`
  - Added per-cause counters in diagnostics panel and render-stats text.

## Behavior Change
- `last_render_error` remains hard-failure-only.
- Throttle behavior is now visible as:
  - total: `last_throttled_render_commands`
  - cause breakdown:
    - `last_throttled_by_capacity_render_commands`
    - `last_throttled_by_interval_render_commands`

## Validation
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8` (pass)
- `pnpm run build` in `MFCMouseEffect/WebUIWorkspace` (pass)

## Next
- Continue Phase 55 manual acceptance closure with real plugin burst input and capture field evidence for throttle/failure separation.
