# Platform Package Build

This folder provides platform-package scaffolding for shell components.

## Targets

- `mfx_shell_windows`
- `mfx_shell_macos`
- `mfx_shell_macos_smoke` (Apple host only; event-loop smoke executable)
- `mfx_shell_macos_tray_smoke` (Apple host only; tray bootstrap smoke executable)
- `mfx_entry_posix_host` (host-only executable for macOS/Linux entry runner)
- `mfx_shell_linux`
- `mfx_shell_posix_common` (shared for macOS/Linux)
- `mfx_entry_runtime_common` (shared entry/runtime bridge)
- `mfx_entry_windows`
- `mfx_entry_posix`

Entry/runtime targets are intentionally single-platform in one configure graph.
If you need `MFX_PACKAGE_PLATFORM=all`, disable entry targets:

```powershell
cmake -S MFCMouseEffect/Platform -B MFCMouseEffect/cmake_build/all-packages `
  -DMFX_PACKAGE_PLATFORM=all `
  -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=OFF
```

Platform package targets and entry/runtime bridge code use `MFX_TARGET_PLATFORM_*`
compile definitions so branch selection follows package target intent instead of host macros.

Cross-host note:
- `macos-shell-vs2026` / `linux-shell-vs2026` on Windows are scaffolding builds for package
  boundary validation.
- Runtime behavior still requires native host verification on the target OS.
- POSIX scaffold entry (`mfx_entry_posix_host`) uses local settings URL convention
  `http://127.0.0.1:9527/?token=scaffold` with embedded scaffold settings server (backed by shared `MouseFx/Server/HttpServer`).
  Static page serving follows Windows WebUI pattern: it prefers existing `WebUI/index.html` + `*.svelte.js`
  from local disk (search roots include repo `MFCMouseEffect/WebUI` and `WebUI`).
  There is no scaffold HTML fallback now; if assets are missing, startup warns and request returns `503`.
  You can force a directory via `MFX_SCAFFOLD_WEBUI_DIR=/abs/path/to/WebUI`.
  The scaffold server exposes:
  - `GET /api/health?token=...`
  - `GET /api/schema?token=...`
  - `GET /api/state?token=...`
  - `POST /api/state?token=...` (in-memory state patch for scaffold UI linkage, no persistence)
  and supports override via
  `MFX_SCAFFOLD_SETTINGS_URL`.

## POSIX Regression

One-command host regression (macOS/Linux):

```bash
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto
```

Notes:
- `--platform macos|linux` can force package target.
- `--build-dir <path>` can reuse existing build output.
- script requires the requested `--platform` to match current host OS.
- The command verifies build, background-exit compatibility, and scaffold HTTP behavior; macOS additionally runs smoke executables.

## Structure

- `Platform/CMakeLists.txt`: package orchestrator
- `Platform/windows/CMakeLists.txt`: windows shell package
- `Platform/macos/CMakeLists.txt`: macos shell package
- `Platform/linux/CMakeLists.txt`: linux shell package
- `Platform/posix/CMakeLists.txt`: shared posix package
- `Platform/posix/Shell/PosixScaffoldAppShell.*`: POSIX scaffold app-shell lifecycle and stdin-exit bridge
- `Platform/posix/Shell/ScaffoldSettingsRuntime.*`: scaffold runtime lifecycle orchestration
- `Platform/posix/Shell/ScaffoldSettingsRequestHandler.*`: scaffold HTTP route dispatch + token gate + in-memory runtime state
- `Platform/posix/Shell/ScaffoldSettingsRouteConfig.*`: scaffold URL/port/path/token parsing helpers
- `Platform/posix/Shell/ScaffoldSettingsApi.*`: scaffold JSON payload + patch validation logic
- `Platform/posix/Shell/ScaffoldSettingsWebUiAssets.*`: WebUI static asset lookup/loading and MIME mapping
- `Platform/windows/Shell/Win32ShellServicesFactory.*`: windows package-local shell service assembly
- `Platform/macos/Shell/MacosShellServicesFactory.*`: macOS package-local shell service assembly
- `Platform/linux/Shell/LinuxShellServicesFactory.*`: linux package-local shell service assembly
- `tools/platform/regression/run-posix-scaffold-regression.sh`: one-command POSIX scaffold regression entry
- `tools/platform/regression/lib/*.sh`: regression subpackages (build/smoke/http/common)

## Presets

`Platform/CMakePresets.json` provides preset entry points:

- `host-auto-vs2026`
- `windows-shell-vs2026`
- `macos-shell-vs2026`
- `linux-shell-vs2026`

Example:

```powershell
cmake --preset windows-shell-vs2026
cmake --build --preset build-windows-shell-release
cmake --build --preset build-windows-entry-release
```

For this repository, full app linking still uses the existing Visual Studio
solution pipeline (`MFCMouseEffect.slnx`).
