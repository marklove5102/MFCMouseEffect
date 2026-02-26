# phase55zzzk: macOS hover overlay style split

## Scope
- Capability bucket: `effects` (macOS hover effect renderer maintainability).
- Goal: split hover-type normalization and color constants from hover overlay lifecycle logic.

## Change Summary
1. Added hover style module:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayStyle.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayStyle.mm`
2. `MacosHoverPulseOverlayRenderer.mm` now delegates:
   - `NormalizeHoverType`
   - glow/tubes palette constants
3. Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt` to include the new style translation unit.

## Contract Invariants
- Hover overlay public APIs remain unchanged:
  - `ShowHoverPulseOverlay`
  - `CloseHoverPulseOverlay`
  - `GetActiveHoverPulseWindowCount`
- `glow/tubes` type behavior and visual semantics remain unchanged.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
