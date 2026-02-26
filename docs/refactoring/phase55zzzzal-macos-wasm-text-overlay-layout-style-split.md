# Phase 55zzzzal - macOS WASM Text Overlay Layout and Style Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.mm` mixed:
  - text overlay layout math
  - panel visual style setup
  - label widget construction
  - render lifecycle orchestration
- Mixed responsibilities increased change risk in WASM text-overlay rendering.

## What Changed
- Added text-overlay internal contract:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.Internal.h`
- Added layout module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.Layout.mm`
- Added style/widget module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.Style.mm`
- Simplified orchestration entry:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmTextOverlay.mm`
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `WASM` (macOS text overlay rendering path).
- Not part of: automation matcher, input-indicator pipeline, non-WASM effect renderers.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
  - `./tools/docs/doc-hygiene-check.sh --strict`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- Behavior contract unchanged; responsibilities split only.
