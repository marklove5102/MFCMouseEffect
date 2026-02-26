# Phase 55zzzzw - macOS App Catalog Bundle Resolve Helper Split

## Why
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.BundleResolve.mm` mixed:
  - bundle entry workflow orchestration
  - process/display name resolve helper internals
- This coupling increases change risk in app-scope catalog normalization path.

## What Changed
- Added bundle-resolve helper contract:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.BundleResolve.Internal.h`
- Added helper implementation module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.BundleResolve.Helpers.mm`
  - owns process name and display name resolution internals.
- Simplified workflow entry:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.BundleResolve.mm`
  - now keeps input checks, URL construction, and helper delegation only.
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Capability Mapping
- This change belongs to: `手势映射/自动化` (app-catalog app-scope resolution pipeline).
- Not part of: effects renderer path, WASM renderer path, input-indicator overlay path.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- No behavior contract change; helper ownership was split only.
