# Phase 55k: macOS WASM Async Task Lifetime Crash Fix

## Issue Classification
- Verdict: `Bug/Regression`.
- Symptom: `mfx_entry_posix_host` could crash with `SIGBUS` during burst click rendering on macOS core lane.
- Evidence: macOS crash report `~/Library/Logs/DiagnosticReports/mfx_entry_posix_host-*.ips` points to main-thread dispatch callback in `RunWasmOverlayOnMainThreadAsync`.

## Root Cause
- `RunWasmOverlayOnMainThreadAsync(const std::function<void()>& task)` captured a reference-backed function object into `dispatch_async` block.
- After function return, async block could execute with a dangling reference, causing use-after-lifetime crash.

## Fix
- Changed overlay runtime main-thread helpers to value-semantics function arguments:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayRuntime.h`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayRuntime.mm`
- Both sync/async paths now move task into a local copied callable before dispatch.
- Result: async closure no longer references expired stack-owned function object.

## Automated Validation (Self-run)
- Precondition: built template plugin and loaded `examples/wasm-plugin-template/dist/plugin.json`.
- Method: auto-start core lane + auto-inject click spam via macOS event API + query `/api/state`.
- Scenario evidence:
  - `baseline`:
    - process alive: `true`
    - `last_render_error`: `""`
    - `last_throttled_render_commands`: `1`
    - `last_throttled_by_capacity_render_commands`: `0`
    - `last_throttled_by_interval_render_commands`: `1`
  - `interval` (`image/text min interval=200ms`):
    - process alive: `true`
    - `last_render_error`: `""`
    - `last_throttled_render_commands`: `1`
    - `last_throttled_by_capacity_render_commands`: `0`
    - `last_throttled_by_interval_render_commands`: `1`
  - `capacity` (`max in-flight=2`):
    - process alive: `true`
    - `last_render_error`: `""`
    - `last_throttled_render_commands`: `1`
    - `last_throttled_by_capacity_render_commands`: `1`
    - `last_throttled_by_interval_render_commands`: `0`

## Regression Gates
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8` (pass)

## Next
- Continue Phase 55 manual acceptance closure on user workflow paths (non-synthetic input + long-run stability).
