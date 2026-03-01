# Phase 56zr - macOS Hold Style Compute ObjC++ Prune

## Scope
- Capability: `effects` (hold category).
- Goal: shrink ObjC++ compile surface without changing hold effect behavior.

## Decision
- Keep `MacosHoldPulseOverlayStyle.cpp` as pure compute module only.
- Move AppKit/CALayer-dependent logic to an existing ObjC++ leaf file (`MacosHoldPulseOverlayRendererCore.Start.cpp`).
- Remove pure compute file from ObjC++ allowlist.

## Code Changes
1. `MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayStyle.cpp`
- Removed `NSColor` conversion and `CAShapeLayer` accent rendering logic.
- Kept only hold type normalization and style resolution.

2. `MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayStyle.h`
- Narrowed API to compute-only contracts (`NormalizeHoldType`, `ResolveHoldStyle`, `HoldStyle`).
- Removed rendering-layer declarations from public style header.

3. `MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayRendererCore.Start.cpp`
- Added local ObjC++ helpers for ARGB->`NSColor`, hold base color selection, and accent-layer configuration.
- Updated call sites to use local helpers instead of style-layer rendering API.

4. `MFCMouseEffect/Platform/macos/Effects/MacosHoldPulseOverlayStyle.Internal.h`
- Added Apple-only forward declarations and CoreGraphics boundary for accent helper ABI.

5. `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- Removed `MacosHoldPulseOverlayStyle.cpp` from `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- ObjC++ allowlist reduced by one additional file.
- `MacosHoldPulseOverlayStyle.cpp` is now C++-only and no longer requires ObjC++ compile mode.
