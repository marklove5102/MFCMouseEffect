# Phase 56q - Scaffold Lane Hold-Follow Normalization Linkage Fix

## Why
- During scaffold-lane regression build, `ScaffoldSettingsApi.StatePatch.cpp` referenced hold-follow normalization symbols that were not linked in that lane.
- This caused `mfx_entry_posix_host` link failures in scaffold package builds.

## What Changed
1. Moved hold-follow normalization logic to header-inline implementation:
- `MouseFx/Core/Config/EffectConfigInternal.h`
- `TryNormalizeHoldFollowMode` and `NormalizeHoldFollowMode` are now inline.

2. Removed duplicate out-of-line definitions:
- `MouseFx/Core/Config/EffectConfig.Internal.cpp`

3. Removed external string helper linkage from inline path:
- inline normalization now uses local ASCII trim/lower logic instead of `ToLowerAscii/TrimAscii` symbols.

## Result
- Both scaffold lane and core lane can use the same normalization contract without extra link dependencies.
- Keeps behavior consistency while restoring scaffold build stability.

## Validation
```bash
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto
./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto
./tools/platform/regression/run-posix-effects-regression-suite.sh --platform auto
```
