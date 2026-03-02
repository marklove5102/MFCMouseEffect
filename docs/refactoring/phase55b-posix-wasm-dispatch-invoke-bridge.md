# Phase55b: POSIX WASM Dispatch Invoke Bridge (Render-Degrade First)

## Goal
- Continue Phase 55 after runtime bootstrap.
- Make WASM event dispatch callable on POSIX core lane (mac first), not Windows-only.
- Keep current render architecture stable: when render backend is unavailable on POSIX, degrade explicitly instead of silently skipping invoke.

## Why this slice
- Phase55a enabled `wasm3_static` backend on macOS, but dispatch path was still Windows-gated.
- Result before this change: runtime backend existed, but click/move/scroll/hold/hover events did not reach WASM host on POSIX.

## Code Changes
1. Dispatch routing opens to cross-platform invoke path
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/WasmDispatchFeature.cpp`
  - Removed Windows-only route stubs for:
    - click / move / scroll / hover-start / hold-start / hold-update / hold-end
  - `IsRouteActive(...)` now uses shared `WasmEffectHost` enabled+loaded checks on all platforms.
  - Non-Windows path now invokes WASM host and records execution diagnostics.

2. POSIX render degradation made explicit
- On non-Windows:
  - command buffer is parsed (`WasmCommandBufferParser`) for observability.
  - rendering is marked unsupported for now.
  - parsed command count is written as dropped commands with explicit `last_render_error`.
- Windows path remains unchanged:
  - still uses `WasmClickCommandExecutor` for actual command rendering.

3. Hold-stream lifecycle remains consistent
- `holdEventActive_/holdButton_` state now follows invoke result on all platforms.
- Reset semantics are unified in one method.

## Behavioral Contract (after this change)
- Windows:
  - unchanged: invoke + execute render commands.
- macOS POSIX core lane:
  - invoke happens when WASM route active.
  - render executes in degraded mode (not supported yet), with clear diagnostics.
  - builtin click fallback behavior remains controlled by existing config (`fallbackToBuiltinClick`).

## Validation
1. Build
- `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
- Result: pass.

2. Core lane gates
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- Result: pass.

3. Non-core regressions
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8`
- `pnpm --dir MFCMouseEffect/WebUIWorkspace run test:automation-platform`
- Result: pass.

## Risks / Next Step
- Current POSIX path is invoke-visible but render-degraded by design.
- Next slice (`phase55c`) should land POSIX render executor abstraction (or platform renderer adapters) so click WASM output can render natively without degrading.
