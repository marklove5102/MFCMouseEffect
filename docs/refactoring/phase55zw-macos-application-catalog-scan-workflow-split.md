# Phase 55zw: macOS Application Catalog Scan Workflow Split

## Capability
- Gesture/action mapping (app-scope catalog path)

## Why
- `MacosApplicationCatalogScanner.mm` mixed façade and full scan workflow implementation.
- Root discovery, bundle parsing, and merge/sort logic were coupled in one file.

## Scope
- Keep app catalog behavior unchanged.
- Extract scan workflow into dedicated module.
- Keep `MacosApplicationCatalogScanner` as thin API entry.

## Code Changes

### 1) New scan workflow module
- Added:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanWorkflow.mm`
- Owns:
  - scan root discovery (`/Applications`, `/System/Applications`, `~/Applications`)
  - bundle process/display name resolution
  - dedup upsert and final sort

### 2) Scanner façade simplification
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanner.mm`
- Now delegates to `ScanMacosApplicationCatalogEntries()`.

### 3) Build wiring
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/CMakeLists.txt`

## Validation
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Contract Impact
- No API/schema behavior change.
- App-scope catalog output semantics remain unchanged.
