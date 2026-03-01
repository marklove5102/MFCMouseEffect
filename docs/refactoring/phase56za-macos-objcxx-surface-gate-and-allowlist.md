# Phase56za: macOS ObjC++ Surface Gate And Allowlist

## Background
- Repository has already removed `.mm` sources, but macOS build still depended on broad ObjC++ compile selectors.
- To keep `mm -> Swift` migration measurable and prevent ObjC++ surface drift, compile policy and regression coverage were tightened.

## What Changed
1. `Platform/macos/CMakeLists.txt`
- Replaced directory wildcard ObjC++ compile matching with explicit source allowlist:
  - `MFX_MACOS_OBJCXX_SOURCE_ALLOWLIST`
- ObjC++ compile flag assignment remains singular and explicit:
  - `COMPILE_FLAGS "-x objective-c++"`
- Removed legacy-path wildcard dependency and renamed include-dir variable to neutral naming:
  - `MFX_MACOS_LEGACY_HEADER_DIRS` -> `MFX_MACOS_PLATFORM_HEADER_DIRS`

2. New regression gate script
- Added:
  - `tools/platform/regression/run-macos-objcxx-surface-regression.sh`
- Gate checks:
  - no `.mm` files in repo
  - no wildcard ObjC++ selectors for `Platform/macos/{legacy,Effects,Overlay,Wasm}`
  - ObjC++ allowlist declaration exists and source selection uses `IN_LIST`
  - ObjC++ compile flag assignment appears exactly once
  - allowlist entries are unique, under expected macOS directories, and resolve to existing files

3. Suite integration
- `tools/platform/regression/run-posix-regression-suite.sh`
  - added ObjC++ surface gate phase execution
- `tools/platform/regression/lib/posix_suite_phases.sh`
  - added `mfx_posix_suite_run_macos_objcxx_surface_gate_phase`
- `tools/platform/regression/lib/posix_suite_options.sh`
  - added skip option:
    - `--skip-macos-objcxx-surface-gate`

## Validation
```bash
cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8
cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto
./tools/platform/regression/run-macos-objcxx-surface-regression.sh
```

## Notes
- This phase is structural policy hardening only; no user-visible behavior change is intended.
- Allowlist is intentionally explicit so future Swift migrations can remove ObjC++ entries file-by-file.
