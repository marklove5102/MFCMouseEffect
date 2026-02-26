# Phase 55zzzb: macOS Effects Category Parity Baseline

## Capability
- Effects

## Why
- macOS core runtime only instantiated click/scroll effects in `EffectFactory`, while trail/hold/hover returned `nullptr`.
- Effect type switching on macOS had weak semantics (`click/scroll` types mapped to one visual path), creating visible behavior drift from Windows.

## Scope
- Keep Windows behavior unchanged.
- Extend macOS effect routing to cover click/trail/scroll/hold/hover categories.
- Keep GPU hold routes out of scope; route to macOS CPU-visible hold implementation.

## Code Changes
1. macOS effect routing expanded in factory:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/EffectFactory.cpp`
2. Existing macOS click/scroll effects updated to keep type semantics:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseEffect.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseEffect.mm`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRenderer.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRenderer.mm`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseEffect.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseEffect.mm`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRenderer.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRenderer.mm`
3. New macOS trail effect path:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEffect.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEffect.mm`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRenderer.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRenderer.mm`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseWindowRegistry.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseWindowRegistry.mm`
4. New macOS hover effect path:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseEffect.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseEffect.mm`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRenderer.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRenderer.mm`
5. New macOS hold effect path:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseEffect.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseEffect.mm`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRenderer.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRenderer.mm`
6. macOS effect sources wired into platform package:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
7. schema capability declaration aligned with macOS effect availability:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.CapabilitiesSections.cpp`
8. test route compatibility updated for new renderer signatures:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestEffectsApiRoutes.cpp`

## Validation
- `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-macos-core-build -DMFX_PACKAGE_PLATFORM=macos -DMFX_ENABLE_POSIX_CORE_RUNTIME=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON`
- `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Contract Impact
- `capabilities.effects.trail/hold/hover` now report `true` on macOS.
- No Windows routing or behavior changes.
- GPU-specific hold routes remain out of scope and are not introduced on macOS.
