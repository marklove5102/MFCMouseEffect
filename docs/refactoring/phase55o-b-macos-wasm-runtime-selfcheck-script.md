# Phase 55o-b: macOS WASM Runtime Selfcheck Script

## Issue Classification
- Verdict: `Engineering efficiency improvement`.
- Problem: phase55o manual closure still depended on repeated ad-hoc API command sequences for:
  - load manifest,
  - enable runtime,
  - dispatch invoke/render verification,
  - fallback-path non-crash verification.

## Goal
1. Provide one-command macOS selfcheck for WASM runtime closure items.
2. Reuse the same host start/probe foundation as manual WebSettings runner.
3. Keep behavior assertions explicit and machine-verifiable.

## Implementation
- Added shared macOS core host helper:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/lib/macos_core_host.sh`
- Refactored WebSettings manual runner to use shared helper:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-core-websettings-manual.sh`
- Added automated selfcheck script:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`

## Selfcheck Coverage
1. `/api/state` confirms `runtime_backend=wasm3_static` and `render_supported=true`.
2. `/api/wasm/load-manifest` with real manifest succeeds.
3. `/api/wasm/enable` succeeds.
4. `/api/wasm/test-dispatch-click` reports:
   - `route_active=true`
   - `invoke_ok=true`
   - `rendered_any=true`
5. Invalid-manifest fallback path is exercised:
   - `/api/wasm/load-manifest` with missing file returns `ok=false`
   - host process remains alive
   - `/api/state` remains responsive after fallback path

## Usage
```bash
./tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh --skip-build
```

Optional:
```bash
./tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh --keep-running --open-settings --auto-stop-seconds 120
```

## Validation
- Local run passed:
  - `./tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh --skip-build`
- Existing contract gate remains green:
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build`
