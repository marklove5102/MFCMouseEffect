# Phase 55zzzze - macOS Global Input Hook EventTap Dispatch Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.EventTap.mm` mixed callback routing and all per-event dispatch bodies.
- Large callback bodies increase coupling and make input-path review harder.

## What Changed
- Added dedicated dispatch implementation unit:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.EventTapDispatch.mm`
- Added class-private handler boundaries in:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.h`
- Simplified callback entry:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.EventTap.mm`
  - now routes by event type and delegates to dedicated handlers.
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `键鼠指示` + `手势映射/自动化触发` input capture event ingress.
- Not part of: WASM render path or effect visual styling.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- Behavior unchanged by design; only callback/dispatch responsibility split.
