# Phase 52e: macOS Coordinate Space Unification for Click/Indicator

## Top Decisions (Important)
- Treat click-position offset on macOS as `Bug或回归`, not a permission issue.
- Fix from architecture root: route macOS click/indicator coordinates through shared `OverlayCoordSpace` contract.
- Keep Windows behavior unchanged; Linux remains compile-level follow.

## Problem Statement
- User verification (2026-02-23):
  - permissions OFF -> degraded warning works (expected),
  - permissions ON -> click effect visible but position offset (unexpected).
- Root cause:
  - macOS click and indicator used raw event points directly for `NSWindow/NSPanel` frame placement.
  - no macOS implementation behind `IOverlayCoordSpaceService` existed, so shared conversion path was effectively Windows-only.

## Changes
1. Added macOS coord-space service implementation:
   - Added `MFCMouseEffect/Platform/macos/Overlay/MacosOverlayCoordSpaceService.h`
   - Added `MFCMouseEffect/Platform/macos/Overlay/MacosOverlayCoordSpaceService.mm`
   - Responsibility:
     - convert Quartz event points to Cocoa screen coordinates (display-aware, with scale mapping),
     - keep existing overlay-origin override contract compatible.
2. Wired platform factory to return macOS coord service:
   - Updated `MFCMouseEffect/Platform/PlatformOverlayCoordSpaceFactory.cpp`
   - Added `Platform/PlatformTarget.h`-based platform dispatch.
3. Routed macOS click effect through shared conversion API:
   - Updated `MFCMouseEffect/Platform/macos/Effects/MacosClickPulseEffect.mm`
   - `OnClick` now uses `ScreenToOverlayPoint(...)` before frame placement.
4. Routed macOS input indicator through shared conversion API:
   - Updated `MFCMouseEffect/Platform/macos/Overlay/MacosInputIndicatorOverlay.mm`
   - relative mode now uses converted point via `ScreenToOverlayPoint(...)`.
   - absolute mode behavior kept unchanged.
5. Build wiring for core lane/link completeness:
   - Updated `MFCMouseEffect/Platform/macos/CMakeLists.txt` (include new macOS coord service source)
   - Updated `MFCMouseEffect/Platform/CMakeLists.txt`:
     - add `PlatformOverlayCoordSpaceFactory.cpp` into runtime common target,
     - add `MouseFx/Core/Overlay/OverlayCoordSpace.cpp` into macOS core lane source set.

## Validation
- macOS core lane compile:
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-macos-core-build -DMFX_PACKAGE_PLATFORM=auto -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON -DMFX_ENABLE_POSIX_CORE_RUNTIME=ON`
  - `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
  - Result: passed
- macOS scaffold regression:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
  - Result: passed
- Linux compile-level follow:
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON`
  - `cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8`
  - Result: passed

## Manual Re-check (execution)
1. Keep both permissions ON (`Accessibility`, `Input Monitoring`).
2. Restart launcher process and app binary.
3. Verify click pulse and input indicator align with cursor (no visible offset).

### Record (2026-02-23, user-verified)
- Result: pass
- Evidence: user反馈“都打开没问题”。

## Risk / Follow-up
- Conversion currently focuses on event-driven click/indicator paths; if other mac visual channels start consuming raw points, they must also use `OverlayCoordSpace`.
- Runtime permission-toggle residual behaviors are tracked in `phase52f-macos-runtime-permission-revocation-hardening.md`.
