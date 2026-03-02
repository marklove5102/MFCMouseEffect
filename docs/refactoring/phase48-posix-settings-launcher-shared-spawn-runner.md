# Phase 48: POSIX Settings Launcher Shared Spawn Runner

## Top Decisions (Important)
- Keep platform launchers (`macos/linux`) thin and move process-launch logic into POSIX shared module.
- Replace shell-string `system(...)` invocation with `posix_spawnp(...)` + `waitpid(...)` to avoid shell quoting pitfalls.
- Keep Windows launcher untouched to avoid cross-platform regression risk.

## Changes
1. Added POSIX shared launcher helper:
   - `Platform/posix/Shell/PosixSettingsLauncher.h`
   - `Platform/posix/Shell/PosixSettingsLauncher.cpp`
2. Updated platform launchers:
   - `Platform/macos/Shell/MacosSettingsLauncher.cpp` now calls `LaunchUrlWithPosixCommand("open", url)`.
   - `Platform/linux/Shell/LinuxSettingsLauncher.cpp` now calls `LaunchUrlWithPosixCommand("xdg-open", url)`.
3. Updated POSIX package build wiring:
   - `Platform/posix/CMakeLists.txt` adds `PosixSettingsLauncher.cpp` into `mfx_shell_posix_common`.

## Behavior
- macOS and Linux keep existing behavior (open settings URL through host command).
- Child process stdout/stderr remains muted (redirected to `/dev/null`), same as previous shell redirection.
- Invalid URL input containing control characters is rejected before process launch.
- URL launch execution no longer depends on shell command composition.

## Validation
- Executed on macOS host:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
- Executed on macOS host for Linux package compile-only check:
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON`
  - `cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8`
- Result:
  - build passed
  - smoke checks passed
  - scaffold HTTP checks passed
  - Linux package static targets compiled successfully on cross-host mode
