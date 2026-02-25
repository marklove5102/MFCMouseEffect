# Phase 55zx: macOS WASM Command Renderer Dispatch Split

## Capability
- WASM

## Why
- `MacosWasmCommandRenderer.mm` mixed orchestration, command dispatch, and resource/color resolution logic.
- Command-type growth would increase coupling and review risk.

## Scope
- Keep renderer behavior unchanged.
- Split renderer into three layers:
  - top-level parse/orchestration
  - command dispatch execution
  - text/image asset/color resolver helpers

## Code Changes

### 1) Resolver helper module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderResolvers.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderResolvers.mm`
- Owns:
  - alpha visibility check
  - text/image path and color/tint fallback resolution

### 2) Dispatch execution module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderDispatch.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderDispatch.mm`
- Owns:
  - parsed-command execution by kind (`spawn_text`, `spawn_image`, `spawn_image_affine`)
  - throttle counters and application to result contract fields

### 3) Renderer entry simplification
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmCommandRenderer.mm`
- Keeps:
  - parse + error handling
  - per-record dispatch call
  - final result aggregation

### 4) Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Contract Impact
- No API/schema behavior change.
- Existing command execution/throttle counters semantics remain unchanged.
