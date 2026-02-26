# Phase 55zzzu - WASM plugin manifest load/validate split

## Summary
- Capability: `wasm` (plugin manifest parsing and validation).
- This slice splits `WasmPluginManifest` into load and validate units, without changing manifest contract semantics.

## Changes
1. Split manifest logic by responsibility
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginManifest.Load.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginManifest.Validate.cpp`
- Removed:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginManifest.cpp`

2. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Runtime now compiles the split units directly.

## Why
- Previous implementation mixed file IO + JSON decoding + validation rules in one file.
- Split keeps validation rules isolated from parse/load mechanics, reducing future change coupling.

## Validation
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/docs/doc-hygiene-check.sh --strict
```

## Compatibility
- No manifest schema changes.
- No error-message or validation-rule changes intended.
