# POSIX Core-Lane Smoke Workflow

## Priority-1 Command
Run this when validating `MFX_ENABLE_POSIX_CORE_RUNTIME=ON` startup/exit stability:

```bash
./tools/platform/regression/run-posix-core-smoke.sh --platform auto
```

## What It Verifies
1. Core runtime lane build is enabled at configure time (`-DMFX_ENABLE_POSIX_CORE_RUNTIME=ON`).
2. `mfx_entry_posix_host` starts successfully in `-mode=background`.
3. Process stays alive for the smoke window (`MFX_CORE_SMOKE_ALIVE_SECONDS`, default `1` second).
4. Process exits on stdin `exit` command within timeout (`MFX_CORE_SMOKE_EXIT_TIMEOUT_SECONDS`, default `5` seconds).

## Core Options
```bash
./tools/platform/regression/run-posix-core-smoke.sh \
  --platform auto \
  --build-dir /tmp/mfx-platform-macos-core-build
```

## Test-Fast Tuning
```bash
MFX_CORE_SMOKE_START_WAIT_SECONDS=1 \
MFX_CORE_SMOKE_ALIVE_SECONDS=1 \
MFX_CORE_SMOKE_EXIT_TIMEOUT_SECONDS=5 \
./tools/platform/regression/run-posix-core-smoke.sh --platform auto
```

## Packaging Layout (Keep This Boundary)
- Orchestrator:
  - `tools/platform/regression/run-posix-core-smoke.sh`
- Shared modules:
  - `tools/platform/regression/lib/build.sh`
  - `tools/platform/regression/lib/core_smoke.sh`
  - `tools/platform/regression/lib/common.sh`
