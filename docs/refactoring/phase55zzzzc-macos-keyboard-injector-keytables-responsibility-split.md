# Phase 55zzzzc - macOS Keyboard Injector KeyTables Responsibility Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyTables.mm` concentrated four mapping domains in one file:
  - printable
  - function keys
  - special keys
  - modifier mapping
- This increased review and regression blast radius for small mapping changes.

## What Changed
- Replaced monolithic implementation with domain-specific units:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyTables.Printable.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyTables.Function.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyTables.Special.mm`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyTables.Modifier.mm`
- Removed old file:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyTables.mm`
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `手势映射/自动化注入` path (keyboard injection keycode mapping).
- Not part of: WASM renderer, native effect visuals, input-indicator visuals.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- Behavior should remain unchanged; mapping tables are moved without contract/schema changes.
