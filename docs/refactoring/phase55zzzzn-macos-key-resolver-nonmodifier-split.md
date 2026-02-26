# Phase 55zzzzn - macOS Key Resolver NonModifier Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyResolver.mm` carried both:
  - modifier mapping route
  - non-modifier fallback chain (printable/function/special)
- Keeping both in one file increases key-resolution coupling.

## What Changed
- Added internal resolver contract:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyResolver.Internal.h`
- Added non-modifier fallback module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyResolver.NonModifier.mm`
- Simplified main resolver:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyResolver.mm`
  - delegates non-modifier path to `resolver_detail::ResolveNonModifierKeyCode`.
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `手势映射/自动化` (keyboard injection key resolution path).
- Not part of: WASM renderer path, input-indicator overlay path, effect style path.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- No behavior contract changes; key-resolver responsibility split only.
