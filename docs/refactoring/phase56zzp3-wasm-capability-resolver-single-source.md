# Phase56zzp3: WASM Capability Resolver Single Source

## Summary
- Category: `WASM`
- Goal: remove duplicated capability decision logic between schema and runtime state output.
- Scope: server capability/state mapping only, no runtime backend behavior change.

## Problem
- `SettingsSchemaBuilder.CapabilitiesSections.cpp` and `SettingsStateMapper.WasmDiagnostics.cpp` each kept local platform capability logic.
- This duplication can drift over time and cause UI/schema and diagnostics/runtime disagreement.

## Change
1. Added shared capability resolver:
   - `MouseFx/Server/SettingsWasmCapabilities.h`
   - `MouseFx/Server/SettingsWasmCapabilities.cpp`
2. Unified both outputs to use the same resolver:
   - `capabilities.wasm.invoke/render` in schema
   - `wasm.invoke_supported/render_supported` in state diagnostics
3. Wired new source into build:
   - `Platform/CMakeLists.txt` adds `SettingsWasmCapabilities.cpp`

## Behavior Impact
- Expected user-visible behavior: unchanged.
- Contract impact: schema and runtime state now share one capability source, reducing future mismatch risk.

## Validation
1. Build:
   - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
2. WASM-focused automation contract:
   - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --check-scope wasm --build-dir /tmp/mfx-platform-macos-build`
3. Full suite:
   - `./tools/platform/regression/run-posix-effects-regression-suite.sh --platform auto`

All passed on macOS host.
