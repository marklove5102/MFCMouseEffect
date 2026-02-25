# Phase 53ah - AppController Dispatch-State Runtime Split

## Background
- `AppController.cpp` still owned dispatch-state helper logic (hover/hold timers, pending hold state, shortcut-capture session handling, cursor/process query helpers).
- These helpers are shared runtime utilities for input indicator and automation dispatch and changed frequently in macOS mainline iterations.
- Keeping them in the main controller file increased cognitive load and made incremental changes riskier.

## Decision
- Extract dispatch-state helpers into a dedicated compilation unit:
  - `AppController.DispatchState.cpp`
- Keep behavior unchanged.
- Keep POSIX and Windows build wiring aligned in the same change.

## Code Changes
1. Added dispatch-state implementation unit
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.DispatchState.cpp`
- Moved implementations from `AppController.cpp`:
  - `ConsumeIgnoreNextClick`
  - `OnGlobalKey`
  - `StartShortcutCaptureSession`
  - `StopShortcutCaptureSession`
  - `PollShortcutCaptureSession`
  - `ConsumeLatestMove`
  - `CurrentTickMs`
  - `CurrentHoldDurationMs`
  - `BeginHoldTracking`
  - `EndHoldTracking`
  - `ArmHoldTimer`
  - `DisarmHoldTimer`
  - `ClearPendingHold`
  - `CancelPendingHold`
  - `ConsumePendingHold`
  - `MarkIgnoreNextClick`
  - `TryEnterHover`
  - `QueryCursorScreenPoint`
  - `CurrentForegroundProcessBaseName`
  - `InjectShortcutForTest`
  - `KillDispatchTimer`

2. Main controller file reduction
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
- Removed moved dispatch-state helpers; retained high-level dispatch activity orchestration and message routing.
- File-size reduction:
  - `AppController.cpp`: 387 -> 221 lines.

3. Cross-platform build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
  - Added `AppController.DispatchState.cpp`.
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MFCMouseEffect.vcxproj`
  - Added `ClCompile` entry for `AppController.DispatchState.cpp`.
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MFCMouseEffect.vcxproj.filters`
  - Added source filter entry.

## Behavior Compatibility
- No API/schema/route changes.
- Input indicator, shortcut capture, hold/hover timer behavior remains unchanged.
- Windows lane behavior unchanged; only compile ownership updated.

## Functional Ownership
- Category: `键鼠指示` + `手势映射`（共享 dispatch-state runtime helpers）。
- Coverage: key handling, shortcut capture session lifecycle, hover/hold state and timer orchestration.

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

2. `./tools/docs/doc-hygiene-check.sh --strict`
- Result: passed.
