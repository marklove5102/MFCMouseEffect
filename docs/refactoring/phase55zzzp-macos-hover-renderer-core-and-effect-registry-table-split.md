# Phase 55zzzp - macOS hover renderer core + effect registry table split

## Summary
- Capability: `effects` (macOS native render path only, no behavior/API contract change).
- This slice continues the renderer/registry "closure" work by splitting two remaining coupling hotspots:
  - `MacosHoverPulseOverlayRenderer`: wrapper vs Objective-C overlay core.
  - `MacosEffectCreatorRegistry`: public entry vs concrete category table/builders.

## Problem
- `MacosHoverPulseOverlayRenderer.mm` still mixed public wrapper flow with Objective-C window/layer construction details.
- `MacosEffectCreatorRegistry.cpp` still mixed factory entry logic with all category creator construction and static registry-table assembly.
- Both patterns increased file churn and review surface when adding/tuning one effect type.

## Changes
1. Hover renderer core split
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.mm`
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRenderer.mm`
- Result:
  - wrapper keeps cross-thread entry/lifecycle boundary;
  - core owns `NSWindow` state and main-thread rendering internals.

2. Effect creator registry table split
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectCreatorRegistry.Internal.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectCreatorRegistry.Table.cpp`
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectCreatorRegistry.cpp`
- Result:
  - public `Create(...)` path stays minimal;
  - concrete category creators and static table wiring are isolated for future extension.

3. Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Added new compilation units for hover renderer core and registry table split.

## Validation
- Full POSIX suite (macOS host + Linux compile gate):
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
```
- Doc hygiene strict:
```bash
./tools/docs/doc-hygiene-check.sh --strict
```

## Risk and Compatibility
- No endpoint/schema/route change.
- No expected runtime behavior change for hover effects or creator selection.
- Windows behavior untouched; Linux remains compile/contract follow lane.
