# Phase 55o: macOS Manual WebSettings Runner

## Issue Classification
- Verdict: `Engineering efficiency improvement`.
- Problem: manual testing on macOS core lane repeatedly required ad-hoc commands for:
  - build/start host,
  - resolving dynamic tokenized settings URL,
  - opening browser and auto-stopping process.

## Goal
1. Provide one stable command for macOS core-lane manual testing.
2. Keep runtime path identical to real tray workflow.
3. Reduce command drift and operator mistakes in repeated acceptance checks.

## Implementation
- Added manual runner script:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-core-websettings-manual.sh`
- Script behavior:
  - optional configure/build for `mfx_entry_posix_host`,
  - force-clean stale `mfx_entry_posix_host` processes,
  - start host in tray mode with probe output,
  - read real tokenized settings URL from probe file,
  - optionally auto-open browser and auto-stop process.
- Key options:
  - `--skip-build`
  - `--auto-stop-seconds <N>` (`0` disables auto stop)
  - `--no-open`
  - `--build-dir`, `--probe-file`, `--log-file`, `--jobs`

## Usage
```bash
./tools/platform/manual/run-macos-core-websettings-manual.sh --auto-stop-seconds 60
```

## Validation
- Script dry-run on macOS host:
  - configure/build/start/probe URL/open path works.
- Existing regression gates remain unchanged:
  - `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build`

## Follow-up
- Companion WASM runtime closure selfcheck:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh`
