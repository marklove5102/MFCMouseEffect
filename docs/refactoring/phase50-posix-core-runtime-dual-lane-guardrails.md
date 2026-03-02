# Phase 50: POSIX Dual Runtime Lane Guardrails

## Top Decisions (Important)
- Keep scaffold lane as default and stable path for POSIX entry runtime.
- Add a separate core lane switch for macOS only; Linux stays scaffold in this phase.
- Split lane selection at compile-time to avoid runtime ambiguity and accidental behavior drift.

## Changes
1. Build switch:
   - Updated `MFCMouseEffect/Platform/CMakeLists.txt`
   - Added `MFX_ENABLE_POSIX_CORE_RUNTIME` (default `OFF`)
2. Entry runtime common sources:
   - Added `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.cpp` into `mfx_entry_runtime_common`
3. Lane routing:
   - Updated `MFCMouseEffect/Platform/PlatformAppShellFactory.cpp`
   - Added `MFX_ENTRY_RUNTIME_POSIX_CORE` branch to instantiate `PosixCoreAppShell`
   - Preserved `MFX_ENTRY_RUNTIME_SCAFFOLD` branch as default
4. New core-lane shell:
   - Added `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.h`
   - Added `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.cpp`
   - Supports startup/tray/background/exit flow and stdin-exit monitor

## Validation
- Scaffold regression (default lane):
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
  - Result: passed (build/smoke/http all passed)
- Core lane configure/build (macOS):
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-macos-core-build -DMFX_PACKAGE_PLATFORM=auto -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON -DMFX_ENABLE_POSIX_CORE_RUNTIME=ON`
  - `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
  - Result: passed
- Core lane startup->exit smoke:
  - `printf 'exit\n' | /tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background`
  - Result: `exit_code:0`

## Risk / Follow-up
- This phase only introduces lane guardrails and shell flow; input/effect runtime is not connected yet.
- Next phase should wire macOS dispatch/input/effect services into the core lane.
