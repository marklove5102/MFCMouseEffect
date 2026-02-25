# Phase 55zzd: macOS WASM Overlay Runtime State Split

## Capability
- WASM

## Why
- `MacosWasmOverlayRuntime.mm` mixed public runtime APIs and internal state machine details:
  - overlay window set/pending count
  - throttle counters/timestamps
  - admission and reset bookkeeping
- This increased coupling between runtime facade and state internals.

## Scope
- Keep runtime behavior unchanged.
- Extract state management into dedicated module.
- Keep runtime file focused on public API forwarding and main-thread execution wrapper.

## Code Changes

### 1) New overlay state module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayState.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayState.mm`
- Owns:
  - slot admission (`capacity + interval`)
  - in-flight/pending window bookkeeping
  - throttle counter tracking
  - reset+drain window-handle extraction for shutdown

### 2) Runtime facade simplification
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmOverlayRuntime.mm`
- Keeps:
  - main-thread sync/async wrappers
  - public API forwarding (`TryAcquire/Release/Register/Take/GetCounters`)
  - `CloseAllWasmOverlayWindows` with main-thread close loop

### 3) Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Contract Impact
- No API/schema behavior change.
- WASM overlay throttle and lifecycle semantics unchanged.
