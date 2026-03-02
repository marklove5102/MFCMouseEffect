# Linux Shell Package

This folder mirrors Flutter-style platform package partitioning.

Implemented in this stage:
- `SettingsLauncher` (shared POSIX spawn runner + Linux `xdg-open` command)
- `SingleInstanceGuard` (POSIX file lock in `/tmp`; cross-host scaffolding build uses process-local stub)
- `EventLoopService` (POSIX blocking loop, no polling, supports `PostTask`)
- `TrayService` (shell layer stub; native appindicator integration pending)
- `UserNotificationService` (`notify-send` + stderr fallback)

Planned next:
- `DpiAwarenessService` equivalent
- Native appindicator/status notifier tray integration
- Native loop bridge (GLib/Qt loop adapter) to replace generic POSIX blocking loop

The concrete classes should implement the cross-platform interfaces in:
- `MouseFx/Core/Shell/*`

Package scaffold target (from `Platform/CMakeLists.txt`):
- `mfx_shell_linux`

Package source wiring:
- `Platform/linux/CMakeLists.txt`
- `Platform/linux/Shell/LinuxShellServicesFactory.*`

Quick host regression from repo root:
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform linux` (run on Linux host)
- verifies package build + entry background exit compatibility + scaffold HTTP route behavior
