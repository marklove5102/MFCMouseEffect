# Phase 42: POSIX Scaffold App Shell Extraction

## Top Decisions (Important)
- Keep `CreatePlatformAppShell` as assembly-only entry; move scaffold runtime shell behavior out of factory.
- Preserve existing startup/runtime behavior (single-instance, tray/background mode, stdin exit monitor, settings URL open).
- Do not change Windows shell path or interface contracts.

## What Changed
1. Added `Platform/posix/Shell/PosixScaffoldAppShell.h`.
2. Added `Platform/posix/Shell/PosixScaffoldAppShell.cpp`.
3. Simplified `Platform/PlatformAppShellFactory.cpp`:
   - retained platform branch selection and default service merge.
   - removed scaffold shell implementation details.
4. Wired new source in `Platform/CMakeLists.txt` (`mfx_entry_runtime_common`).

## Responsibility Split
- `Platform/PlatformAppShellFactory.cpp`
  - platform shell selection
  - default service merge
  - object creation orchestration
- `Platform/posix/Shell/PosixScaffoldAppShell.*`
  - scaffold POSIX shell lifecycle
  - tray/background startup branch
  - stdin exit monitor
  - settings open/exit request bridge
- `Platform/posix/Shell/ScaffoldSettingsRuntime.*`
  - scaffold HTTP runtime lifecycle and settings URL serving

## Behavior Compatibility
- Preserved:
  - `exit` and `{\"cmd\":\"exit\"}` background stdin termination behavior.
  - tray mode settings opening via scaffold URL.
  - scaffold API/static route behavior and token gate.
- No Windows behavior path changed by this refactor.

## Validation
- Build:
  - `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host mfx_shell_macos_smoke mfx_shell_macos_tray_smoke -j8`
- Smoke:
  - `mfx_shell_macos_smoke` => `0`
  - `mfx_shell_macos_tray_smoke` => `0`
  - background stdin `exit` / json-exit => `0`
- HTTP regression:
  - root/js/state/post => `200`
  - bad token static route => `403`
  - state payload still reports `platform:\"macos\"`.

## Follow-up
- Completed in Phase 43:
  - Introduced one-command POSIX scaffold regression script under `tools/platform/regression/`.
