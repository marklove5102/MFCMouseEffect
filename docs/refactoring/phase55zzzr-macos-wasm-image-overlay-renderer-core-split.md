# Phase 55zzzr - macOS WASM image overlay renderer core split

## Summary
- Capability: `wasm` (image overlay render path).
- This slice continues renderer closure by splitting `MacosWasmImageOverlayRenderer` into wrapper/core layers, keeping behavior unchanged.

## Changes
1. Added core renderer units
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmImageOverlayRendererCore.h`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmImageOverlayRendererCore.mm`

2. Slimmed public wrapper unit
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Wasm/MacosWasmImageOverlayRenderer.mm`
- Result:
  - wrapper keeps stable public entry (`RenderWasmImageOverlay`);
  - Objective-C window/layer/render internals now live in core.

3. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Added new core source to macOS shell target.

## Why
- The previous file mixed public boundary and heavy render internals in one unit.
- Splitting reduces churn risk in M2 WASM render evolution while preserving runtime contracts.

## Validation
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/docs/doc-hygiene-check.sh --strict
```

## Compatibility
- No API/route/schema changes.
- No expected behavior change in render/throttle/admission flow.
- Windows behavior untouched, Linux compile lane unchanged.
