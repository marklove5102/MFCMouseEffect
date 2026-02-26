# phase55zzzo: hold and trail renderer core split

## Scope
- Capability bucket: `effects` (macOS renderer maintainability).
- Goal: split hold/trail renderer files into thin wrappers plus dedicated core implementation units.

## Change Summary
1. Added trail core module:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.mm`
2. Added hold core module:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.mm`
3. Slimmed wrapper files:
   - `MacosTrailPulseOverlayRenderer.mm`
   - `MacosHoldPulseOverlayRenderer.mm`
4. Updated build wiring:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Contract Invariants
- Public APIs unchanged:
  - `ShowTrailPulseOverlay` / `CloseAllTrailPulseWindows`
  - `StartHoldPulseOverlay` / `UpdateHoldPulseOverlay` / `StopHoldPulseOverlay` / `GetActiveHoldPulseWindowCount`
- Effect visuals, lifecycle, and state update semantics unchanged.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`
