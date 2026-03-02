# Phase 55i: macOS WASM Throttle Diagnostics + Linux Roadmap Decision

## Goal
- Make WASM render failure paths easier to triage on macOS core lane.
- Explicitly freeze Linux renderer scope for current M2 to avoid ambiguous expectations.

## Implementation
- Upgraded macOS transient overlay return contract from boolean to explicit status:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTransientOverlay.h`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTransientOverlay.mm`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.mm`
  - New status: `Rendered` / `Throttled` / `Failed`.
- Updated macOS WASM renderer aggregation:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderer.mm`
  - Tracks throttled drops separately from other dropped commands.
  - Keeps `last_render_error` for hard failures only (throttling is reported by counters, not error text).
- Extended shared render result + host diagnostics:
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmCommandExecutionResult.h`
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmEffectHost.h`
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmEffectHost.cpp`
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmEventInvokeExecutor.cpp`
  - Added `throttledCommands` and `lastThrottledRenderCommands`.
- Exposed diagnostics in state APIs:
  - `MFCMouseEffect/MouseFx/Server/SettingsStateMapper.cpp`
  - `MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
  - `MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.cpp`
  - New field: `last_throttled_render_commands`.
- Regression contract update:
  - `tools/platform/regression/lib/core_http.sh`
  - Core HTTP regression now asserts `last_throttled_render_commands` exists in `/api/state`.

## Linux Renderer Decision (Current M2)
- Decision: keep Linux renderer as `degrade-only` for current Phase 55 scope.
- Contract impact remains unchanged:
  - `invoke_supported = true`
  - `render_supported = false`
- Rationale:
  - keep cross-platform contract stable while macOS renderer closes operational hardening,
  - avoid introducing Linux runtime/render backend complexity in the same phase,
  - preserve small-step commit rhythm and lower regression risk.

## Behavior Change
- macOS `/api/state` and `/api/wasm/state` now expose throttled-drop diagnostics separately, so burst-policy drops no longer look identical to hard render failures.
- `last_render_error` is now reserved for actual render failures; throttle-only drops keep error text empty and rely on `last_throttled_render_commands`.
- Linux visible behavior is unchanged by this phase (decision documentation only).

## Validation
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8` (pass)

## Next
- Completed in Phase 55j: throttle diagnostics now distinguish capacity vs interval causes and are visible in API/WebUI.
- Remaining: manual acceptance closure with real plugin bursts on macOS.
