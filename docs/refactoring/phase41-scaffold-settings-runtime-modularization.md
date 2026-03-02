# Phase 41: Scaffold Settings Runtime Modularization

## Top Decisions (Important)
- Keep public runtime API unchanged: `ScaffoldSettingsRuntime` still exposes `SetRuntimeMode/Start/Stop/SettingsUrl`.
- Split scaffold internals by responsibility to avoid large-file coupling and ease future extension.
- Preserve all existing runtime behavior and route contracts to prevent cross-platform regression.

## New Module Boundaries
1. `Platform/posix/Shell/ScaffoldSettingsRouteConfig.*`
   - URL parsing and token/query/path normalization.
   - Embedded server eligibility decision from `MFX_SCAFFOLD_SETTINGS_URL`.
2. `Platform/posix/Shell/ScaffoldSettingsWebUiAssets.*`
   - WebUI directory resolution (`MFX_SCAFFOLD_WEBUI_DIR` + default search roots).
   - Static file loading and content-type mapping.
3. `Platform/posix/Shell/ScaffoldSettingsApi.*`
   - Scaffold JSON payload builders (`health/schema/state/error`).
   - State patch parsing and field validation.
4. `Platform/posix/Shell/ScaffoldSettingsRequestHandler.*`
   - HTTP request routing.
   - Token gate, API dispatch, static fallback, in-memory runtime state guard.
5. `Platform/posix/Shell/ScaffoldSettingsRuntime.cpp`
   - Lifecycle orchestration only (`HttpServer` start/stop + warning bridge).

## Why This Is Better
- Single responsibility per file is now explicit and enforceable.
- Changes in one concern (for example WebUI path policy) no longer require touching API or server lifecycle code.
- Review and regression scope is reduced because module ownership is clearer.

## Behavior Compatibility Check
- Default route and token policy unchanged: `http://127.0.0.1:9527/?token=scaffold`.
- Custom route override unchanged with `MFX_SCAFFOLD_SETTINGS_URL`.
- Missing WebUI behavior unchanged:
  - root returns `503`
  - missing static asset returns `404`
  - response message remains explicit build guidance.
- API compatibility unchanged:
  - `GET /api/health`
  - `GET /api/schema`
  - `GET /api/state`
  - `POST /api/state`

## Validation
- Build:
  - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host mfx_shell_macos_smoke mfx_shell_macos_tray_smoke -j8`
- Smoke:
  - `mfx_shell_macos_smoke` exit code `0`
  - `mfx_shell_macos_tray_smoke` exit code `0`
  - background stdin `exit` text/json command exit code `0`
- HTTP regression:
  - default route root/js/schema/state GET/POST all `200`
  - bad token on static route `403`
  - custom route (`/ui/settings?token=dev`) root/js/state POST `200`
  - forced missing WebUI root `503`, js `404`

## File Size Result
- `ScaffoldSettingsRuntime.cpp`: `97` lines
- Largest new scaffold module cpp: `217` lines

## Follow-up
- Completed in Phase 42:
  - POSIX scaffold app-shell behavior moved out of `PlatformAppShellFactory.cpp` into `PosixScaffoldAppShell.*`.
