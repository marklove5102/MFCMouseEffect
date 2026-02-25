# Phase 53ab - Automation Dispatch Decoupling (Matcher/Executor Boundary)

## Background
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Automation/InputAutomationEngine.cpp` previously mixed:
  - input trigger orchestration
  - action-history maintenance
  - binding matching
  - keyboard shortcut injection
- This made gesture/mouse automation evolution tightly coupled to engine internals.

## Decision
- Introduce a dedicated dispatch module that owns matcher/executor flow.
- Keep `InputAutomationEngine` focused on input-event orchestration and runtime state.
- Preserve existing behavior contracts and route-level APIs.

## Code Changes
1. Added dispatch boundary module
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Automation/InputAutomationDispatch.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Automation/InputAutomationDispatch.cpp`
- New ownership:
  - normalized action-id handling
  - action-history append/prune by chain timing limits
  - best-enabled-binding selection
  - shortcut injection execution

2. Reduced engine responsibilities
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Automation/InputAutomationEngine.cpp`
- `TriggerMouseAction(...)` and `TriggerGesture(...)` now delegate to `automation_dispatch::DispatchAction(...)`.
- Removed engine-local duplicated matcher/executor helpers.

- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Automation/InputAutomationEngine.h`
- Removed private helper declarations that moved into the dispatch module.

3. Build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added `InputAutomationDispatch.cpp` to core runtime sources.

## Behavior Compatibility
- Mouse and gesture mapping trigger semantics are unchanged.
- Foreground process filtering and chain-priority matching remain unchanged.
- This phase is structure-only refactor with explicit matcher/executor boundary.

## Functional Ownership
- Category: `手势映射`
- Coverage: mouse/gesture automation dispatch path (`history -> match -> inject`).

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

2. `./tools/docs/doc-hygiene-check.sh --strict`
- Result: passed.
