# Phase 55zzzw - WASM effect host invoke split

## Summary
- Capability: `wasm` host execution path.
- This slice separates invoke-path logic from lifecycle/load logic in `WasmEffectHost`.

## Changes
1. Added invoke-focused implementation unit
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmEffectHost.Invoke.cpp`
- Ownership:
  - invoke-path diagnostics reset;
  - `InvokeEvent` execution, parse, budget decisions;
  - `BuildEventInputV1`.

2. Slimmed main host unit
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmEffectHost.cpp`
- Main unit now keeps lifecycle/load/reload/unload and general diagnostics helpers.

3. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added invoke unit to runtime sources.

## Why
- Original file mixed plugin lifecycle/load concerns with invoke fast-path and budget handling.
- Split reduces coupling and keeps runtime hot-path changes isolated.

## Validation
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/docs/doc-hygiene-check.sh --strict
```

## Compatibility
- No API/schema changes.
- No behavior changes intended for WASM invoke/load flows.
