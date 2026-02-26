# phase55zzzn: click and scroll renderer core split

## Scope
- Capability bucket: `effects` (macOS renderer maintainability).
- Goal: split click/scroll renderer files into thin API wrappers plus dedicated core implementation units.

## Change Summary
1. Added click core module:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRendererCore.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRendererCore.mm`
2. Added scroll core module:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.mm`
3. Slimmed wrapper files:
   - `MacosClickPulseOverlayRenderer.mm`
   - `MacosScrollPulseOverlayRenderer.mm`
4. Updated build wiring:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Contract Invariants
- Public APIs unchanged:
  - `ShowClickPulseOverlay` / `CloseAllClickPulseWindows`
  - `ShowScrollPulseOverlay` / `CloseAllScrollPulseWindows`
- Effect visuals and lifecycle behavior unchanged.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`
