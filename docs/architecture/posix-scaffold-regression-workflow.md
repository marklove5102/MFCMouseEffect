# POSIX Scaffold Regression Workflow

## Priority-1 Command
Run this before/after POSIX scaffold changes:

```bash
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto
```

For full POSIX gate (scaffold + linux compile + automation semantic tests), use:

```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
```

`--platform auto` always resolves to current host (`macos` or `linux`).
Cross-host regression run is intentionally blocked to avoid false pass.

Optional:

```bash
./tools/platform/regression/run-posix-scaffold-regression.sh --platform macos --build-dir /tmp/mfx-platform-macos-build
./tools/platform/regression/run-posix-scaffold-regression.sh --platform linux --build-dir /tmp/mfx-platform-linux-build
```

Related compile-level gate:

```bash
./tools/platform/regression/run-posix-linux-compile-gate.sh
```

## What It Verifies
1. Platform package configure/build (`mfx_entry_posix_host` + platform targets).
2. Background stdin exit compatibility:
   - `exit`
   - `{"cmd":"exit"}`
3. macOS host smoke binaries (when available):
   - `mfx_shell_macos_smoke`
   - `mfx_shell_macos_tray_smoke`
   - tray smoke additionally asserts `Settings` action callback and launcher URL call-path via test-only env/probe files.
4. Scaffold HTTP behavior:
   - default route and token gate
   - custom `MFX_SCAFFOLD_SETTINGS_URL`
   - missing WebUI fallback (`503` root, `404` asset)
   - automation API contracts in scaffold lane:
     - `POST /api/automation/active-process` must return `404 not found` (explicitly unsupported in scaffold mode)
     - `POST /api/automation/app-catalog` must return `404 not found` (explicitly unsupported in scaffold mode)

## Packaging Layout (Keep This Boundary)
- Entry orchestrator:
  - `tools/platform/regression/run-posix-scaffold-regression.sh`
- Build step:
  - `tools/platform/regression/lib/build.sh`
- Smoke step:
  - `tools/platform/regression/lib/smoke.sh`
- HTTP regression step:
  - `tools/platform/regression/lib/http.sh`
- Shared assertions/log helpers:
  - `tools/platform/regression/lib/common.sh`

## Cross-Platform Policy
1. macOS and Linux share POSIX regression entry and HTTP checks.
2. macOS runs extra smoke executables.
3. Linux currently skips platform-specific smoke executable (not provided yet) but still verifies build + entry/runtime HTTP behavior.

## Extension Rule
When adding a new check:
1. Put it into the matching single-purpose module under `tools/platform/regression/lib`.
2. Keep `run-posix-scaffold-regression.sh` orchestration-only.
3. If behavior/expectation changes, update this doc and `docs/refactoring/*` in the same commit.
