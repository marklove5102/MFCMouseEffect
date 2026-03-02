# Phase 55h: macOS WASM Overlay Throttle Guardrail

## Goal
- Reduce overlay churn risk under burst WASM render output on macOS core lane.
- Keep behavior predictable with explicit runtime policy and test-friendly overrides.

## Implementation
- Added dedicated overlay policy module:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayPolicy.h`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayPolicy.cpp`
- Added runtime admission control in overlay runtime:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayRuntime.h`
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayRuntime.mm`
  - New capabilities:
    - in-flight cap (`active windows + pending create requests`)
    - per-kind minimum spawn interval (`image` / `text`)
    - pending-slot release path for window creation failure
- Wired admission control into render entry points:
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTransientOverlay.mm` (`image`)
  - `MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.mm` (`text`)
- Build wiring:
  - `MFCMouseEffect/Platform/macos/CMakeLists.txt` adds `MacosWasmOverlayPolicy.cpp`.

## Runtime Policy (Production + Test)
- `MFX_MACOS_WASM_OVERLAY_MAX_INFLIGHT`
  - default: `64`
  - clamp: `[8, 512]`
  - meaning: max in-flight overlays (created + pending).
- `MFX_MACOS_WASM_IMAGE_MIN_INTERVAL_MS`
  - default: `4`
  - clamp: `[0, 1000]`
  - meaning: minimum admission interval for image overlays.
- `MFX_MACOS_WASM_TEXT_MIN_INTERVAL_MS`
  - default: `8`
  - clamp: `[0, 1000]`
  - meaning: minimum admission interval for text overlays.

### Test-value examples
- Stress-drop mode (easy to observe throttling):
  - `MFX_MACOS_WASM_OVERLAY_MAX_INFLIGHT=8`
  - `MFX_MACOS_WASM_IMAGE_MIN_INTERVAL_MS=40`
  - `MFX_MACOS_WASM_TEXT_MIN_INTERVAL_MS=80`
- Near-production mode (low behavior impact while still guarded):
  - use defaults (do not set env vars).

### Switching
- Env vars are read at process startup.
- Toggle by setting/unsetting env vars before launching `mfx_entry_posix_host`.

## Behavior Change
- On macOS core lane, burst render requests now degrade by dropping excessive overlay requests instead of unbounded transient window growth.
- No Windows behavior change.
- Linux remains compile/contract follow and does not enable this AppKit overlay path.

## Validation
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8`

## Risks
- Current guardrail is policy-based admission, not full overlay pooling.
- Very long `delayMs` commands still occupy pending slots until they execute or fail.

## Next
- Completed in Phase 55i: render diagnostics now split throttled-vs-failed drops, and Linux current-M2 renderer roadmap is explicitly locked as degrade-only.
- Remaining: manual plugin burst acceptance closure on macOS.
