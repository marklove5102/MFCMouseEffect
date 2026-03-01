# Phase 56zy - macOS Hover Core Wrapper ObjC++ Prune

## Scope
- Capability: `effects` (hover category).
- Goal: keep hover core wrapper C++-only and confine ObjC++ to layers leaf.

## Decision
- Keep `MacosHoverPulseOverlayRendererCore.cpp` as pure wrapper/orchestration.
- Move command-path AppKit rendering and active-window lifecycle to `MacosHoverPulseOverlayRendererCore.Layers.cpp`.
- Remove `MacosHoverPulseOverlayRendererCore.cpp` from ObjC++ allowlist.

## Code Changes
1. `MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.cpp`
- Removed AppKit/CALayer dependencies.
- Kept compute-wrapper overload:
  - `ShowHoverPulseOverlayOnMain(overlayPt, effectType, themeName, profile)`.

2. `MFCMouseEffect/Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Layers.cpp`
- Added/owned command-path renderer:
  - `ShowHoverPulseOverlayOnMain(const HoverEffectRenderCommand&, const std::string&)`.
- Added/owned overlay lifecycle:
  - `CloseHoverPulseOverlayOnMain()`
  - `GetActiveHoverPulseWindowCountOnMain()`.
- Kept ring/tubes layer construction in ObjC++ leaf.

3. `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Removed `MacosHoverPulseOverlayRendererCore.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- ObjC++ allowlist reduced from `10` to `9`.
- Hover core wrapper file is now C++-only; ObjC++ remains in layers leaf.
