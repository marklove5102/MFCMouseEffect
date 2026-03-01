# phase56zzp19: macOS wasm tint alpha parity fix

## Context
- Verdict: `Bug/regression`.
- macOS tint bridge initially applied alpha twice for tinted plugin images:
  - once in tint color generation (`mfxColorFromArgb(..., alphaScale)`)
  - once via `imageView.alphaValue = alphaScale`
- This produced darker output than Windows (`runtimeAlpha` squared).

## Changes
1. Updated Swift tint path:
- `Platform/macos/Wasm/MacosWasmImageOverlayBridge.swift`
- For `applyTint=true`, tint color now uses scale `1.0`:
  - `mfxColorFromArgb(tintArgb, 1.0)`
- Global runtime alpha still applied through `imageView.alphaValue`.

2. Effective semantics after fix:
- `finalAlpha ~= tintA * runtimeAlpha`
- aligns with Windows color-matrix path intent (`effectiveAlpha * tintA`) and avoids `runtimeAlpha^2` attenuation.

## Validation
1. Core WASM contract:
```bash
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --check-scope wasm --build-dir /tmp/mfx-platform-macos-core-automation-build
```

2. macOS wasm selfcheck:
```bash
./tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh --skip-build --build-dir /tmp/mfx-platform-macos-core-automation-build
```

## Result
- macOS tinted plugin-image brightness/alpha behavior is now aligned with Windows semantic baseline.
