# Phase 55g: macOS WASM Render Fidelity (Assets + Kinematics)

## Goal
- Improve macOS WASM renderer fidelity beyond basic visible fallback.
- Keep architecture maintainable by splitting overlay runtime responsibilities.

## Implementation
- Upgraded macOS image command execution path:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderer.mm`
  - `spawn_image` / `spawn_image_affine` now build `WasmImageOverlayRequest` and pass:
    - `delayMs`
    - `vx/vy/ax/ay`
    - `rotation`
    - `imageId`-mapped asset path (when available)
- Added plugin image-asset resolution in mac renderer:
  - Uses `WasmPluginImageAssetCatalog::ResolveImageAssetPath(activeManifestPath, imageId, ...)`
  - Falls back to tint-based pulse rendering when asset is missing/unloadable.
- Added unified image overlay request contract:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTransientOverlay.h`
  - New `WasmImageOverlayRequest` captures visual + motion fields.
- Refactored mac overlay runtime for single responsibility:
  - Added `MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayRuntime.h`
  - Added `MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayRuntime.mm`
  - Moved main-thread dispatch + overlay-window registry out of transient renderer file.
  - Added `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.mm` to isolate text-overlay rendering from image-overlay path.
  - `MacosWasmTransientOverlay.mm` now focuses on image overlay path and no longer hosts text rendering.
- Build wiring:
  - `MFCMouseEffect/Platform/macos/CMakeLists.txt` now includes `MacosWasmOverlayRuntime.mm`.
  - `MFCMouseEffect/Platform/CMakeLists.txt` adds `WasmPluginImageAssetCatalog.cpp` for POSIX core lane link closure.

## Behavior Changes
- macOS WASM image commands now support:
  - delayed start (`delayMs`)
  - trajectory displacement derived from velocity/acceleration
  - rotation animation on overlay layer
  - plugin image asset preferred rendering when manifest assets exist
- If asset resolution fails, renderer degrades to pulse visual instead of hard fail.
- `render_supported` capability behavior remains strategy-driven and unchanged from 55f (`true` on current macOS strategy).

## Validation
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto` (pass)
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8` (pass)

## Risks
- Overlay window churn can still grow under high command burst; this phase improves fidelity, not throughput shaping.
- Asset rendering currently relies on transient AppKit image views; advanced blending/affine parity with Windows remains partial.

## Next
- Phase 55h:
  - close manual acceptance for real plugin asset cases on macOS,
  - add light throttling/pooling strategy if overlay churn appears,
  - finalize Linux renderer roadmap decision and document contract implications.
