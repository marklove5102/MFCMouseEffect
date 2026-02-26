# phase55zzze: SettingsStateMapper effects diagnostics split

## Scope
- Capability bucket: `effects` (diagnostics/runtime observability).
- Goal: reduce `SettingsStateMapper.Diagnostics.cpp` coupling without changing runtime behavior.

## Change Summary
1. Added `MFCMouseEffect/MouseFx/Server/SettingsStateMapper.EffectsDiagnostics.cpp`.
2. Moved these functions to the new file:
   - `BuildEffectsRuntimeState`
   - `BuildEffectsProfileState`
3. Kept function signatures and output schema unchanged.
4. Updated POSIX CMake source list:
   - `MFCMouseEffect/Platform/CMakeLists.txt` now includes `SettingsStateMapper.EffectsDiagnostics.cpp`.

## Why
- `SettingsStateMapper.Diagnostics.cpp` mixed three domains:
  - GPU/WASM diagnostics
  - input-capture diagnostics
  - effects runtime/profile diagnostics
- Splitting effects diagnostics lowers file complexity and keeps boundaries explicit for future macOS effects evolution.

## Contract Invariants
- `/api/state.effects_runtime` fields stay unchanged.
- `/api/state.effects_profile` fields stay unchanged.
- No route path or payload compatibility break.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `pnpm --dir MFCMouseEffect/WebUIWorkspace run test:effects-profile-model`
- `pnpm --dir MFCMouseEffect/WebUIWorkspace run build:effects`
