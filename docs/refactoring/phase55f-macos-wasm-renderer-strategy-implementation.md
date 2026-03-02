# Phase 55f: macOS WASM Renderer Strategy Implementation

## Goal
- Move macOS from WASM render-degrade to actual visible render output under the `IWasmCommandRenderer` contract.
- Keep Windows behavior unchanged and keep Linux compile/contracts stable.

## Implementation
- Added platform renderer implementation:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderer.h`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderer.mm`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTransientOverlay.h`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTransientOverlay.mm`
  - Responsibility split: command parsing/execution and transient overlay rendering are separated.
- Updated platform renderer factory:
  - `MFCMouseEffect/MouseFx/Core/Wasm/WasmCommandRenderer.cpp`
  - macOS branch now returns `CreateMacosWasmCommandRenderer()`.
- Updated macOS package build wiring:
  - `MFCMouseEffect/Platform/macos/CMakeLists.txt` adds `MacosWasmCommandRenderer.mm`.
  - `MFCMouseEffect/Platform/macos/CMakeLists.txt` adds `MacosWasmTransientOverlay.mm`.
- Updated capability signaling:
  - `MFCMouseEffect/MouseFx/Server/SettingsStateMapper.cpp`
  - `render_supported` now comes from the platform renderer strategy (`SupportsRendering()`), not hardcoded platform `#if`.
- Updated regression contract:
  - `tools/platform/regression/lib/core_http.sh`
  - macOS now asserts `"render_supported":true`.

## Render Scope (Current)
- macOS renderer now executes these WASM command kinds with visible output:
  - `spawn_image`
  - `spawn_image_affine` (uses base image fields)
  - `spawn_text`
- Current rendering uses native transient AppKit overlays (pulse/text windows).
- This phase does not yet implement plugin image-asset raster fidelity parity with Windows `WasmImageFileRenderer`.

## Behavior Impact
- Windows: unchanged (`WasmClickCommandExecutor` path retained).
- macOS: WASM render path is no longer degrade-only; command output can render.
- Linux: still degrade-only strategy and remains compile-level + contract-level follow.

## Validation
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8` (pass)

## Risks
- macOS renderer currently prioritizes command visibility and stability; advanced asset fidelity/animation parity is still pending.
- Frequent WASM command bursts may increase transient window churn; monitor in manual acceptance.

## Next
- Completed in Phase 55g: renderer fidelity now includes asset-backed image preference and kinematics mapping.
- Remaining: manual plugin acceptance closure and burst-load operational hardening.
