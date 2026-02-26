# phase55zzzh: macOS trail overlay style split

## Scope
- Capability bucket: `effects` (macOS trail effect renderer maintainability).
- Goal: split trail-style normalization/color/path construction from trail overlay lifecycle logic.

## Change Summary
1. Added trail style module:
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayStyle.h`
   - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayStyle.mm`
2. Simplified `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRenderer.mm` to focus on:
   - overlay window lifecycle
   - animation wiring
   - window registry/cleanup
3. Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt` to include the new trail style translation unit.

## Contract Invariants
- Trail overlay public APIs remain unchanged:
  - `CloseAllTrailPulseWindows`
  - `ShowTrailPulseOverlay`
- Trail type normalization and visual style mapping semantics remain unchanged.

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
