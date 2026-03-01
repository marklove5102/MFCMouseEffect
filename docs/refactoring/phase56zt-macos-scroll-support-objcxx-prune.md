# Phase 56zt - macOS Scroll Support ObjC++ Prune

## Scope
- Capability: `effects` (scroll category).
- Goal: reduce ObjC++ compile surface by moving renderer-only logic into an existing ObjC++ leaf.

## Decision
- Keep `MacosScrollPulseOverlayRendererSupport.cpp` as pure compute/support module.
- Move `CAShapeLayer` construction and ARGB->`NSColor` usage to `MacosScrollPulseOverlayRendererCore.cpp`.
- Remove `MacosScrollPulseOverlayRendererSupport.cpp` from ObjC++ allowlist.

## Code Changes
1. `MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererSupport.cpp`
- Removed AppKit/QuartzCore dependency and all layer-creation helpers.
- Kept only:
  - `ResolveStrengthLevel`
  - `BuildBodyRect`
  - `BuildPulseDuration`
  - `BuildCloseAfterMs`

2. `MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererSupport.h`
- Removed `CAShapeLayer`-related declarations.
- Header now exposes support compute API only.

3. `MFCMouseEffect/Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.cpp`
- Added local ObjC++ helper functions:
  - `CreateBodyLayer(...)`
  - `CreateArrowLayer(...)`
- Added style header include for `CreateScrollPulseDirectionArrowPath`.
- Updated call sites to use local helpers instead of support layer factory calls.

4. `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Removed `MacosScrollPulseOverlayRendererSupport.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- ObjC++ allowlist reduced from `15` to `14`.
- Scroll support module is now C++-only; ObjC/AppKit remains at renderer leaf boundaries.
