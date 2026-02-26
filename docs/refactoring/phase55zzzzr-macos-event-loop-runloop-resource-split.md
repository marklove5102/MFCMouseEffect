# Phase 55zzzzr - macOS Event Loop RunLoop Resource Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosEventLoopService.cpp` mixed:
  - service flow (`Run`/`RequestExit`/`PostTask`/drain)
  - runloop resource lifecycle (`create/add/remove/release/wakeup`)
- This coupling increases shell-lifecycle change risk.

## What Changed
- Added runloop lifecycle module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosEventLoopService.RunLoop.cpp`
  - owns `RunLoopSourcePerform`, `SetupRunLoopLocked`, `TeardownRunLoopLocked`, `SignalRunLoopLocked`.
- Updated service header:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosEventLoopService.h`
  - declared runloop lifecycle helpers explicitly.
- Simplified service flow file:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosEventLoopService.cpp`
  - now focuses on high-level control flow and task-drain behavior.
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `键鼠指示 + 手势映射/自动化 + 特效/WASM` shared shell runtime entry (common event-loop substrate).
- It is infrastructure-only; no feature contract changes.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- No user-visible behavior change; runloop resource ownership only was reorganized.
