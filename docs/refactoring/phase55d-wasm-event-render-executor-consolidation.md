# Phase 55d: WASM Event/Render Executor Consolidation

## Background
- Before this change, WASM event invoke and render bookkeeping existed in two places:
  - `WasmDispatchFeature::TryInvokeAndRender`.
  - `AppController::OnDispatchActivity` hover-end branch.
- The two paths duplicated host invoke/record logic and increased drift risk for POSIX degradation behavior.

## Decision
- Introduce a shared event execution boundary in `MouseFx/Core/Wasm`.
- Keep platform-specific rendering decisions in a dedicated renderer boundary instead of embedding `#if` branches inside control flow code.

## Implementation
- Added `MouseFx/Core/Wasm/WasmCommandExecutionResult.h`.
  - Extracts shared WASM render execution result contract from click-specific executor header.
- Added `MouseFx/Core/Wasm/WasmCommandRenderer.h` and `MouseFx/Core/Wasm/WasmCommandRenderer.cpp`.
  - Introduces `IWasmCommandRenderer` strategy boundary for platform-specific render behavior.
  - Windows strategy delegates to existing `WasmClickCommandExecutor`.
  - POSIX strategy parses command buffer and reports explicit render-degrade diagnostics without rendering.
- Added `MouseFx/Core/Wasm/WasmEventInvokeExecutor.h` and `MouseFx/Core/Wasm/WasmEventInvokeExecutor.cpp`.
  - Owns `InvokeEvent -> command renderer -> RecordRenderExecution` pipeline.
  - Returns structured execution result (`routeActive/invokeOk/render`).
- Updated `MouseFx/Core/Control/WasmDispatchFeature.cpp`.
  - Removed duplicated platform render branch and direct host record writes.
  - Delegates to `wasm::InvokeEventAndRender`.
- Updated `MouseFx/Core/Control/AppController.cpp`.
  - Hover-end WASM path now reuses shared executor, no local duplicate invoke/render branch.
- Build integration:
  - Updated POSIX CMake target sources in `MFCMouseEffect/Platform/CMakeLists.txt`.
  - Updated Windows project source/header lists in `MFCMouseEffect/MFCMouseEffect.vcxproj` and `MFCMouseEffect/MFCMouseEffect.vcxproj.filters`.

## Behavior Impact
- Windows behavior remains on the existing renderer path (`WasmClickCommandExecutor`).
- POSIX behavior remains degrade-only for rendering, but diagnostics and bookkeeping now use one shared pipeline.
- Hover-end now follows the same shared invoke path as other WASM events, reducing event-path divergence.

## Validation
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8` (pass)

## Risks
- Shared executor is now a new dependency for both dispatch route and hover-end path; regression risk is concentrated but easier to test.
- POSIX rendering remains not implemented (degrade-only). This phase is structural preparation for full POSIX render backend.

## Next Step
- Completed by Phase 55e: renderer strategy boundary (`IWasmCommandRenderer`) is now in place.
- Remaining: implement real POSIX render strategy (Phase 55f) so `render_supported` can transition from `false` to `true` on supported platforms.
