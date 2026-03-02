# Phase 52: macOS Input/Dispatch Foundation (M1 Base)

## Top Decisions (Important)
- Land macOS runtime primitives first (dispatch host/codec/input hook/cursor/indicator), then connect higher-level core routing in the next slice.
- Keep scaffold lane behavior unchanged; all new macOS runtime components are additive and isolated behind platform factories.
- Maintain Linux compile-level follow by keeping non-mac branches on existing null contracts.

## Changes
1. Added macOS dispatch runtime primitives:
   - `MFCMouseEffect/Platform/macos/Control/MacosDispatchMessageHost.h`
   - `MFCMouseEffect/Platform/macos/Control/MacosDispatchMessageHost.cpp`
   - `MFCMouseEffect/Platform/macos/Control/MacosDispatchMessageCodec.h`
   - `MFCMouseEffect/Platform/macos/Control/MacosDispatchMessageCodec.cpp`
   - Features:
     - dedicated worker-thread dispatch queue
     - async/sync posting contract
     - timer support for hover/hold style dispatch ids
2. Added macOS input services:
   - `MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.h`
   - `MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.mm`
   - `MFCMouseEffect/Platform/macos/System/MacosCursorPositionService.h`
   - `MFCMouseEffect/Platform/macos/System/MacosCursorPositionService.cpp`
   - Features:
     - CGEventTap-based global move/click/scroll/key capture
     - move coalescing support through `ConsumeLatestMove`
     - keyboard capture exclusive gate for shortcut-capture mode
3. Added macOS indicator and click baseline visuals:
   - `MFCMouseEffect/Platform/macos/Overlay/MacosInputIndicatorOverlay.h`
   - `MFCMouseEffect/Platform/macos/Overlay/MacosInputIndicatorOverlay.mm`
   - `MFCMouseEffect/Platform/macos/Effects/MacosClickPulseEffect.h`
   - `MFCMouseEffect/Platform/macos/Effects/MacosClickPulseEffect.mm`
4. Wired macOS factories:
   - `MFCMouseEffect/Platform/PlatformControlServicesFactory.cpp`
   - `MFCMouseEffect/Platform/PlatformControlMessageCodecFactory.cpp`
   - `MFCMouseEffect/Platform/PlatformInputServicesFactory.cpp`
   - `MFCMouseEffect/Platform/PlatformOverlayServicesFactory.cpp`
5. Build integration:
   - `MFCMouseEffect/Platform/macos/CMakeLists.txt`
     - compiles all new macOS runtime sources
     - links `ApplicationServices` and `QuartzCore`
   - `MFCMouseEffect/Platform/CMakeLists.txt`
     - compiles platform runtime factory units in entry runtime common target

## Validation
- Scaffold lane regression:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
  - Result: passed
- macOS core lane build + startup/exit smoke:
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-macos-core-build -DMFX_PACKAGE_PLATFORM=auto -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON -DMFX_ENABLE_POSIX_CORE_RUNTIME=ON`
  - `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
  - `printf 'exit\n' | /tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background`
  - Result: passed (`exit_code:0`)
- Linux compile-level follow:
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON`
  - `cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8`
  - Result: passed

## Risk / Follow-up
- This slice lands the macOS runtime components and factory wiring; full `AppController/DispatchRouter` core-lane activation is still a follow-up slice.
- Permission-guided degradation UX (explicit user-facing warning path) should be completed when core lane starts consuming `MacosGlobalInputHook`.
