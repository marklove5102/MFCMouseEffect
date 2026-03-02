# Phase 51: Core Win32 Decoupling and POSIX Path Foundation

## Top Decisions (Important)
- Remove Win32-only assumptions from core control/config contracts first, then layer platform implementations.
- Keep Windows runtime behavior unchanged while opening compile/runtime space for macOS/Linux.
- Prioritize contract compatibility: new fields and signatures must remain backward-compatible with existing flows.

## Changes
1. Dispatch and hook contracts:
   - Updated `MFCMouseEffect/MouseFx/Core/Control/IDispatchMessageHost.h`
     - Added `PostAsync(...)` for thread-safe async event delivery.
   - Updated `MFCMouseEffect/MouseFx/Core/System/IGlobalMouseHook.h`
     - `Start(...)` now accepts `IDispatchMessageHost*` instead of platform handle.
   - Updated Windows implementation:
     - `MFCMouseEffect/Platform/windows/Control/Win32DispatchMessageHost.h`
     - `MFCMouseEffect/Platform/windows/Control/Win32DispatchMessageHost.cpp`
     - `MFCMouseEffect/Platform/windows/System/Win32GlobalMouseHook.h`
     - `MFCMouseEffect/Platform/windows/System/Win32GlobalMouseHook.cpp`
2. AppController message semantics:
   - Updated `MFCMouseEffect/MouseFx/Core/Control/AppController.h`
   - Updated `MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
   - Updated `MFCMouseEffect/MouseFx/Core/Control/DispatchRouter.cpp`
   - `OnDispatchActivity` now uses `DispatchMessageKind + timerId` instead of direct `WM_TIMER` checks.
   - `ERROR_INVALID_HANDLE` dependency replaced by platform-agnostic fallback constant.
3. Config/runtime path decoupling:
   - Updated `MFCMouseEffect/Platform/PlatformRuntimeEnvironment.h`
   - Updated `MFCMouseEffect/Platform/PlatformRuntimeEnvironment.cpp`
   - Updated `MFCMouseEffect/Platform/windows/System/Win32RuntimeEnvironment.h`
   - Updated `MFCMouseEffect/Platform/windows/System/Win32RuntimeEnvironment.cpp`
   - Added `GetPreferredConfigDirectoryW()` with per-platform implementation.
   - Updated `MFCMouseEffect/MouseFx/Core/Config/ConfigPathResolver.cpp` to consume platform runtime env APIs instead of direct `shlobj.h`.
4. Path joining and app-scope normalization:
   - Updated `MFCMouseEffect/MouseFx/Core/Config/EffectConfig.Load.cpp`
   - Updated `MFCMouseEffect/MouseFx/Core/Config/EffectConfig.Save.cpp`
   - Config file paths now use `std::filesystem::path` composition.
   - Updated `MFCMouseEffect/MouseFx/Core/Config/EffectConfig.Internal.cpp`
   - `.exe` auto-append is now Windows-only.
5. Cross-platform key semantics:
   - Updated `MFCMouseEffect/MouseFx/Core/Protocol/InputTypes.h` (`KeyEvent.meta`)
   - Updated `MFCMouseEffect/MouseFx/Core/Automation/KeyChord.cpp` (`cmd/command` modifier alias)

## Validation
- macOS scaffold lane:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
  - Result: passed
- macOS core lane compile:
  - `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
  - Result: passed
- Linux compile-level guard (cross-host on macOS):
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON`
  - `cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8`
  - Result: passed

## Risk / Follow-up
- Core lane currently validates startup/exit shell behavior, but full macOS input/effect pipeline is still pending.
- Next phase should land macOS dispatch host, global input hook, indicator overlay, and basic click effect mapping.
