# phase56w - macOS click text route to Windows semantic path

## Decision
- Classify issue as `Bug/regression`: macOS `click=text` rendered as pulse ring with `LEFT/RIGHT/MIDDLE`, not floating text semantics.
- Use the same semantic effect class as Windows: `TextEffect`.
- Keep platform renderer responsibility in fallback layer only.

## Changes
1. Effect route:
   - `Platform/macos/Effects/MacosEffectCreatorRegistry.Table.cpp`
   - map click type `text` to `TextEffect` instead of `MacosClickPulseEffect`.
2. macOS fallback implementation:
   - `Platform/macos/Effects/MacosTextEffectFallback.h`
   - `Platform/macos/Effects/MacosTextEffectFallback.mm`
   - transparent text-only panel, float-up + sway + fade animation.
3. Platform fallback factory:
   - `Platform/PlatformEffectFallbackFactory.cpp`
   - return `MacosTextEffectFallback` on macOS.
4. Build wiring:
   - `Platform/macos/CMakeLists.txt`
   - add `MacosTextEffectFallback.mm` and `Settings/EmojiUtils.cpp`.

## Risk
- Text animation now depends on per-click transient `NSPanel`; high-frequency clicks can increase short-lived window churn.
- WASM remains higher-priority for click routing when enabled and plugin renders output; this change affects built-in `click=text` path.

## Validation
1. Build:
   - `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
2. Contract:
   - `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
3. Manual:
   - set click effect to `text`;
   - confirm no ring + no `LEFT/RIGHT/MIDDLE` label;
   - confirm configured text content floats upward and fades.
