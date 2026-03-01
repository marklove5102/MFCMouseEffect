# POSIX Core Automation Contract Workflow

## Primary Command
Use this as the default gate for macOS core-lane automation contracts:

```bash
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto
```

## Contract Coverage
The script validates these groups end-to-end:
1. Boot + probe lifecycle:
- core lane build flag is on (`MFX_ENABLE_POSIX_CORE_RUNTIME=ON`)
- runtime URL/token probe works
- settings-launch probe captures launcher call path

2. Core HTTP/API behavior:
- settings shell and state endpoints are reachable
- auth contract for `/api/state` (`401` without token, `200` with token)
- automation endpoints work (`active-process`, `app-catalog`, scope persistence)
- test-only endpoints validate:
  - app-scope alias matching (`code`, `code.app`, `code.exe`)
  - binding priority (`process > all` at same chain length, longest-chain-first)
  - matcher + injector path
  - mac keycode mapping (`Cmd+V` vs `Cmd+Tab`)
  - injector dry-run probe

3. Permission degrade/recovery:
- startup denied permission converges to degraded state
- degraded warning is deduplicated
- runtime revoke/regrant converges without process restart
- input-indicator labels remain valid (`L/R/M`)

4. WASM route contract:
- catalog/import endpoints are available
- import-folder probe-only route works without modal dialog
- test-dispatch click route is active and renders
- state capability/runtime fields remain consistent
  (`runtime_backend=wasm3_static`, `render_supported=true`)

5. macOS semantic guards:
- app catalog non-empty and includes `.app` process suffix
- selected app-scope survives state roundtrip
- `capabilities.input.keyboard_injector=true`

## Common Options
```bash
./tools/platform/regression/run-posix-core-automation-contract-regression.sh \
  --platform auto \
  --build-dir /tmp/mfx-platform-macos-core-automation-build
```

## Fast Probe Tuning
```bash
MFX_CORE_HTTP_START_WAIT_SECONDS=1 \
MFX_CORE_HTTP_PROBE_TIMEOUT_SECONDS=8 \
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto
```

## Test Gates (Environment Flags)
The script enables these by default and can be overridden to `0` for local debugging:
- `MFX_ENABLE_WASM_TEST_DISPATCH_API`
- `MFX_ENABLE_AUTOMATION_SCOPE_TEST_API`
- `MFX_ENABLE_AUTOMATION_SHORTCUT_TEST_API`
- `MFX_ENABLE_AUTOMATION_INJECTION_TEST_API`
- `MFX_ENABLE_INPUT_INDICATOR_TEST_API`
- `MFX_TEST_KEYBOARD_INJECTOR_DRY_RUN` (deterministic injector probe)

Script-managed probe files:
- `MFX_CORE_WEB_SETTINGS_LAUNCH_PROBE_FILE`
- `MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE`
- `MFX_TEST_INPUT_CAPTURE_PERMISSION_SIM_FILE`
- `MFX_TEST_NOTIFICATION_CAPTURE_FILE`
- `MFX_CORE_HTTP_INPUT_CAPTURE_TIMEOUT_SECONDS` (default `10`)

## Ownership Boundaries
- Orchestrator:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
- Shared modules:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/build.sh`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/common.sh`
