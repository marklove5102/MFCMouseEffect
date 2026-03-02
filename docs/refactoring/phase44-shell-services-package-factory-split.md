# Phase 44: Shell Services Package Factory Split

## Top Decisions (Important)
- Keep `PlatformShellServicesFactory` as dispatch-only entry.
- Move shell service object assembly into each platform package (`windows/macos/linux`).
- Preserve runtime behavior; this is architecture cleanup only.

## New Files
- `Platform/windows/Shell/Win32ShellServicesFactory.h`
- `Platform/windows/Shell/Win32ShellServicesFactory.cpp`
- `Platform/macos/Shell/MacosShellServicesFactory.h`
- `Platform/macos/Shell/MacosShellServicesFactory.cpp`
- `Platform/linux/Shell/LinuxShellServicesFactory.h`
- `Platform/linux/Shell/LinuxShellServicesFactory.cpp`

## Updated Files
- `Platform/PlatformShellServicesFactory.cpp`
  - removed direct platform service includes
  - now dispatches to package-local factory by `MFX_PLATFORM_*`
- `Platform/windows/CMakeLists.txt`
- `Platform/macos/CMakeLists.txt`
- `Platform/linux/CMakeLists.txt`

## Responsibility Split
- `Platform/PlatformShellServicesFactory.cpp`
  - select package factory by platform macro
- `Platform/*/Shell/*ShellServicesFactory.*`
  - construct concrete tray/settings/single-instance/dpi/event-loop/notifier services

## Why This Helps
- Platform-specific service assembly no longer accumulates in one cross-platform file.
- New platform package onboarding has a single local extension point.
- Public entry stays stable while implementation remains package-owned.

## Validation
- Executed:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
- Result:
  - configure/build passed
  - smoke checks passed
  - scaffold HTTP regression passed

## Follow-up
- Completed in Phase 45:
  - macOS shell package landed native host loop/tray implementation files and smoke runners.
