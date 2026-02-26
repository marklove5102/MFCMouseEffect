# Phase 55zzzx - DispatchRouter pointer/timer path split

## Summary
- Capability: `effects` + `automation mapping` + `input indicator` + `wasm` dispatch routing path.
- This slice splits `DispatchRouter` by responsibility, isolating pointer/button/timer flows from main route entry.

## Changes
1. Added internal helper boundary
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/DispatchRouter.Internal.h`
- Provides shared routing helpers (`MessagePoint`, known timer-id check) across split units.

2. Added pointer/timer routing unit
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/DispatchRouter.Pointer.cpp`
- Owns:
  - `OnMove`
  - `OnScroll`
  - `OnButtonDown`
  - `OnButtonUp`
  - `OnTimer`

3. Slimmed main routing unit
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/DispatchRouter.cpp`
- Keeps:
  - route entry (`Route`)
  - click/key path (`OnClick`, `OnKey`)
  - high-level message kind dispatch.

4. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`

## Why
- Original `DispatchRouter.cpp` mixed all message kinds, making pointer/timer changes high-risk.
- Split reduces coupling and keeps hot pointer/timer path changes localized.

## Validation
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/docs/doc-hygiene-check.sh --strict
```

## Compatibility
- No message kind behavior changes intended.
- Existing dispatch contracts remain unchanged.
