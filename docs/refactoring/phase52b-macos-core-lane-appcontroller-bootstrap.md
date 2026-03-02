# Phase 52b: macOS Core Lane AppController Bootstrap

## Top Decisions (Important)
- Keep scaffold lane default behavior unchanged; bootstrap AppController only in macOS core lane (`MFX_ENABLE_POSIX_CORE_RUNTIME=ON`).
- Preserve Windows behavior; non-Windows paths use explicit degradation for unavailable hook/WASM/GDI+ parts instead of process abort.
- Keep Linux at compile-level follow and avoid pulling macOS-only runtime pieces into Linux targets.

## Changes
1. Posix core lane now starts real controller:
   - Updated `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.h`
   - Updated `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.cpp`
   - `PosixCoreAppShell` now owns `AppController` and starts/stops it with shell lifecycle.
2. Core runtime source wiring (macOS core lane only):
   - Updated `MFCMouseEffect/Platform/CMakeLists.txt`
   - Added core/control/config/automation/wasm-minimum sources to `mfx_entry_runtime_common` only when:
     - target platform is macOS
     - `MFX_ENABLE_POSIX_CORE_RUNTIME=ON`
3. Non-Windows degradations to keep core lane alive:
   - Updated `MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
   - Updated `MFCMouseEffect/MouseFx/Core/Control/AppController.Wasm.cpp`
   - Updated `MFCMouseEffect/MouseFx/Core/Control/WasmDispatchFeature.cpp`
   - Updated `MFCMouseEffect/MouseFx/Core/Control/CommandHandler.cpp`
   - Hook start failure on non-Windows no longer hard-aborts startup.
   - WASM dispatch/runtime paths become no-op on non-Windows in this phase.
4. Remove remaining hard Windows-only compile blockers for core lane:
   - Updated `MFCMouseEffect/MouseFx/Core/System/GdiPlusSession.cpp` (non-Windows stub startup/shutdown)
   - Updated `MFCMouseEffect/MouseFx/Core/Control/EffectFactory.cpp` (macOS click-first mapping to `MacosClickPulseEffect`)
   - Updated `MFCMouseEffect/MouseFx/Core/Overlay/OverlayHostService.h`
   - Updated `MFCMouseEffect/MouseFx/Core/Overlay/OverlayHostService.cpp` (non-Windows no-op overlay service path)
   - Updated `MFCMouseEffect/MouseFx/Renderers/Hold/Presentation/QuantumHaloPresenterSelection.cpp` (cross-platform env lookup)

## Validation
- macOS core lane compile:
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-macos-core-build -DMFX_PACKAGE_PLATFORM=auto -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON -DMFX_ENABLE_POSIX_CORE_RUNTIME=ON`
  - `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
  - Result: passed
- macOS core lane startup/exit smoke:
  - `printf 'exit\n' | /tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background`
  - Result: `exit_code:0`
- scaffold lane regression:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
  - Result: passed
- Linux compile-level follow:
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON`
  - `cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8`
  - Result: passed

## Risk / Follow-up
- Current core lane is now bootstrapped with AppController and input pipeline primitives, but this phase still keeps non-Windows WASM dispatch as degraded no-op.
- Permission UX text and settings-driven capability exposure should be completed in next slice to satisfy full phase acceptance visibility.
