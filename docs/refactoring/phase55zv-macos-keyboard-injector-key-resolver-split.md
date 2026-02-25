# Phase 55zv: macOS Keyboard Injector Key Resolver Split

## Capability
- Gesture/action mapping (automation shortcut injection)

## Why
- `MacosKeyboardInjector.mm` mixed two layers:
  - shortcut key resolution (`vk -> mac keycode + flags`)
  - key event posting/injection flow
- This increased coupling and made key mapping evolution harder to review.

## Scope
- Keep injection behavior unchanged.
- Extract key-resolution logic into dedicated module.
- Keep `MacosKeyboardInjector` focused on chord execution flow.

## Code Changes

### 1) New key resolver module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyResolver.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjectorKeyResolver.mm`
- Owns:
  - modifier mapping resolution
  - printable/function/special key resolution
  - unified `ResolveKeyCode` and modifier-flag output

### 2) Keyboard injector flow-only implementation
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosKeyboardInjector.mm`
- Keeps:
  - chord parsing + execution order
  - dry-run gate
  - accessibility trust gate
  - key event post sequence (modifier down -> key down/up -> modifier up)

### 3) Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Added resolver implementation to `mfx_shell_macos`.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- No API/schema changes.
- No injection behavior changes.
- Internal decoupling only for safer future shortcut mapping expansion.
