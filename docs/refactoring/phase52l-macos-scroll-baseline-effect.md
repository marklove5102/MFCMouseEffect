# Phase 52l - macOS Scroll Baseline Effect

## Background
- macOS core lane had click-visible baseline effect, but scroll category still resolved to `nullptr` in `EffectFactory` for POSIX.
- Default config uses `active.scroll="arrow"`, so users expected visible scroll feedback but got no effect on macOS.
- Web settings capability schema also marked `capabilities.effects.scroll=false` on macOS, keeping the UI in degraded state.

## Decision
- Add a lightweight native macOS scroll-visible baseline effect (`MacosScrollPulseEffect`).
- Keep behavior simple and safe:
  - no extra global permissions
  - no change to Windows semantics
  - no change to Linux runtime scope (still compile/contract follow)
- Update schema capability to expose scroll support on macOS.

## Code Changes
1. New macOS scroll effect implementation
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseEffect.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseEffect.mm`
- Effect behavior:
  - renders a transient direction pulse window on scroll
  - supports vertical/horizontal direction cues
  - direction and intensity are derived from `ScrollEvent::delta` and `ScrollEvent::horizontal`

2. Factory mapping
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/EffectFactory.cpp`
- macOS mapping now includes:
  - `EffectCategory::Click -> MacosClickPulseEffect`
  - `EffectCategory::Scroll -> MacosScrollPulseEffect`

3. macOS shell build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Added `MacosScrollPulseEffect.mm` to `mfx_shell_macos` source list.

4. Schema capability update
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.CapabilitiesSections.cpp`
- Changed `capabilities.effects.scroll` to true on macOS.

## Behavior Compatibility
- Windows behavior unchanged.
- Linux runtime behavior unchanged (still out of current M1 runtime scope).
- macOS only: scroll now has baseline visible effect instead of silent no-op.

## Functional Ownership
- Category: `特效`
- Coverage: M1 baseline visible effect expansion on macOS (`click-only` -> `click + scroll`).

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

2. `./tools/docs/doc-hygiene-check.sh --strict`
- Result: passed.
