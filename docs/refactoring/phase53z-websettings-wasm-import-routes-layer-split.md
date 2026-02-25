# Phase 53z - WebSettings WASM Import Routes Layer Split

## Background
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmImportRoutes.cpp` mixed two route responsibilities:
  - `/api/wasm/import-selected`
  - `/api/wasm/import-from-folder-dialog`
- This increased review surface for route updates and made import-source-specific changes harder to isolate.

## Decision
- Keep public route entry `HandleWebSettingsWasmImportApiRoute(...)` unchanged.
- Split import handling by source ownership:
  - selected manifest import route
  - native folder dialog import route
- Keep `WebSettingsServer.WasmImportRoutes.cpp` as delegating entry only.

## Code Changes
1. Added selected-manifest route module
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmImportSelectedRoute.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmImportSelectedRoute.cpp`
- Owns `/api/wasm/import-selected` parsing, transfer call, and response serialization.

2. Added folder-dialog route module
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmImportFolderDialogRoute.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmImportFolderDialogRoute.cpp`
- Owns `/api/wasm/import-from-folder-dialog` handling including:
  - `probe_only` branch
  - native picker call
  - `plugin.json` existence validation
  - transfer call and response serialization

3. Reduced entry file to delegator
- Replaced implementation in `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.WasmImportRoutes.cpp`
- Entry now dispatches to selected-route module first, then folder-dialog module.

4. Build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added:
  - `WebSettingsServer.WasmImportSelectedRoute.cpp`
  - `WebSettingsServer.WasmImportFolderDialogRoute.cpp`

## Behavior Compatibility
- API paths and payload schemas are unchanged.
- `probe_only` contract and dialog-import error handling are unchanged.
- This phase is a structure-only split for lower coupling and easier maintenance.

## Functional Ownership
- Category: `WASM`
- Coverage: WebSettings WASM import control-plane routes (`selected` + `folder-dialog`).

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

2. `./tools/docs/doc-hygiene-check.sh --strict`
- Result: passed.
