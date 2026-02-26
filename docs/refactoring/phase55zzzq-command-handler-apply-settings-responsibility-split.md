# Phase 55zzzq - CommandHandler apply_settings responsibility split

## Summary
- Capability scope: cross-cutting settings ingestion path (covers `effects`, `input indicator`, `automation mapping`, `wasm` payload fields).
- Goal: reduce coupling in `apply_settings` without changing HTTP/API/runtime behavior.

## Changes
1. Added internal contract header
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/CommandHandler.ApplySettings.Internal.h`
- Defines shared helper boundary used by split translation units.

2. Split apply_settings implementations by responsibility
- Entry orchestration retained in:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/CommandHandler.ApplySettings.cpp`
- Visual/effects/input parsing moved to:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/CommandHandler.ApplySettings.Visual.cpp`
- Automation and WASM settings parsing moved to:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/CommandHandler.ApplySettings.AutomationWasm.cpp`

3. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Included new translation units in runtime target sources.

## Why this closure matters
- `CommandHandler.ApplySettings.cpp` was a cross-domain monolith (active effect selection + trail tuning + input indicator + automation + wasm budgets).
- Splitting by domain lowers regression blast radius for future macOS-first changes while keeping Windows and Linux contracts unchanged.

## Validation
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/docs/doc-hygiene-check.sh --strict
```

## Compatibility
- No route/schema changes.
- No change in settings key semantics.
- Existing regression suite and selfchecks remain green.
