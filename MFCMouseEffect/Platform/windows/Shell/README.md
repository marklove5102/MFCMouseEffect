# Windows Shell Package

This folder contains the Windows shell package implementation:

- `TrayService` (`Win32TrayService` + tray host window/menu)
- `SettingsLauncher` (`ShellExecuteW`)
- `SingleInstanceGuard` (named mutex)
- `DpiAwarenessService`
- `EventLoopService` (`GetMessageW` loop + posted task queue)
- `UserNotificationService`
- `Win32AppShell` bridge for `AppShellCore`

For cross-platform package orchestration, `Platform/CMakeLists.txt` exposes:

- `mfx_shell_windows`

Package source wiring now lives in:
- `Platform/windows/CMakeLists.txt`

The package target is a shell-layer static library scaffold. Full app linking
still uses the existing Visual Studio solution pipeline.

Package-local service assembly:
- `Platform/windows/Shell/Win32ShellServicesFactory.*`
