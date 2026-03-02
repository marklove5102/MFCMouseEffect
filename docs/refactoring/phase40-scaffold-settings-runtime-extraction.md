# Phase 40: Scaffold Settings Runtime Extraction

## Summary
Refactor POSIX scaffold settings logic out of `PlatformAppShellFactory.cpp` into a dedicated runtime module to enforce single responsibility and reduce coupling.

## Why This Refactor
- `PlatformAppShellFactory.cpp` had mixed responsibilities: app-shell composition, scaffold route parsing, static asset serving, API state patching.
- Large in-file logic increased review cost and made regressions harder to isolate.
- The scaffold settings lifecycle is now a reusable runtime concern, not factory glue.

## Key Changes
1. Added `Platform/posix/Shell/ScaffoldSettingsRuntime.h`.
2. Added `Platform/posix/Shell/ScaffoldSettingsRuntime.cpp`.
3. Reduced `PlatformAppShellFactory.cpp` to orchestration only.
4. Wired new module into `mfx_entry_runtime_common` in `Platform/CMakeLists.txt`.

## Responsibility Split
- `PlatformAppShellFactory.cpp`
  - Platform app-shell assembly
  - Tray/startup/shutdown sequencing
  - stdin exit monitor
  - Open-settings bridge call
- `ScaffoldSettingsRuntime.cpp`
  - `MFX_SCAFFOLD_SETTINGS_URL` route parsing
  - `MFX_SCAFFOLD_WEBUI_DIR` resolution
  - Static WebUI asset serving (`index.html` + `*.svelte.js`)
  - Token gate (`token` query check)
  - Scaffold APIs:
    - `GET /api/health`
    - `GET /api/schema`
    - `GET /api/state`
    - `POST /api/state` (in-memory patch)

## Behavioral Compatibility
- Kept existing scaffold URL convention: `http://127.0.0.1:9527/?token=scaffold`.
- Kept token protection for static and API routes.
- Kept in-memory state patch semantics for `POST /api/state`.
- Kept no-HTML-fallback policy:
  - missing root entry returns `503` with explicit build hint.

## Validation
- Build targets:
  - `mfx_entry_posix_host`
  - `mfx_shell_macos_smoke`
  - `mfx_shell_macos_tray_smoke`
- Runtime checks passed:
  - background `exit` text/json command
  - tray settings page serves Svelte assets
  - scaffold API GET/POST works
  - bad token returns `403`
  - empty `MFX_SCAFFOLD_WEBUI_DIR` returns root `503`

## Risks
- Runtime route/state logic moved to a new module; future changes should be done there first.
- If `WebUI` artifacts are missing, entry page intentionally fails fast (`503`).

## Follow-up
- Completed in Phase 41:
  - `ScaffoldSettingsRouteConfig.*`
  - `ScaffoldSettingsApi.*`
  - `ScaffoldSettingsWebUiAssets.*`
  - `ScaffoldSettingsRequestHandler.*`
