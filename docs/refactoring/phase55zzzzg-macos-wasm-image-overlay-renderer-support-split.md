# Phase 55zzzzg - macOS WASM Image Overlay Renderer Support Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmImageOverlayRendererCore.mm` still held utility logic (`alpha/delay clamp`, `motion check`, `path conversion`) mixed with render flow.
- Utility and render flow coupling increases maintenance risk.

## What Changed
- Added support module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmImageOverlayRendererSupport.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmImageOverlayRendererSupport.mm`
- Simplified core renderer:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmImageOverlayRendererCore.mm`
  - utility calls now delegate to support module.
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `WASM` image command rendering path.
- Not part of: input-indicator path, automation mapping path, native effect style path.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- No contract/schema changes; utility extraction only.
