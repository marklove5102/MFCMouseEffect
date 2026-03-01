# Phase 56zx - macOS Click Core Wrapper ObjC++ Prune

## Scope
- Capability: `effects` (click category).
- Goal: keep click core wrapper file C++-only and confine ObjC++ to layers leaf.

## Decision
- Keep `MacosClickPulseOverlayRendererCore.cpp` as pure wrapper/orchestration.
- Move command-path AppKit rendering into `MacosClickPulseOverlayRendererCore.Layers.cpp`.
- Remove `MacosClickPulseOverlayRendererCore.cpp` from ObjC++ allowlist.

## Code Changes
1. `MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRendererCore.cpp`
- Removed AppKit/CALayer/dispatch dependencies.
- Kept compute-wrapper overload:
  - `ShowClickPulseOverlayOnMain(overlayPt, button, effectType, themeName, profile)`.

2. `MFCMouseEffect/Platform/macos/Effects/MacosClickPulseOverlayRendererCore.Layers.cpp`
- Added command-path renderer:
  - `ShowClickPulseOverlayOnMain(const ClickEffectRenderCommand&, const std::string&)`.
- Added required includes:
  - `MacosClickPulseOverlayRendererCore.h`
  - `MacosClickPulseWindowRegistry.h`
  - `dispatch/dispatch.h`

3. `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Removed `MacosClickPulseOverlayRendererCore.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- ObjC++ allowlist reduced from `11` to `10`.
- Click core wrapper file is now C++-only; ObjC++ remains in layers leaf.
