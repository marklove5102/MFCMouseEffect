# Phase55a: macOS WASM3 Runtime Bootstrap (M2 First Slice)

## Goal
- Start Phase 55 on macOS core lane with a real WASM runtime backend.
- Keep Windows runtime behavior unchanged (still dynamic bridge first).
- Add regression visibility so runtime backend drift is caught automatically.

## Design Decisions (Windows-thought aligned)
- Keep platform runtime selection inside `WasmRuntimeFactory` + `PlatformWasmRuntimeFactory`.
- Preserve Windows default path:
  - `dynamic_bridge` remains primary backend on Windows.
  - No fallback promotion to wasm3 on Windows in this phase.
- Enable POSIX core lane runtime:
  - macOS uses `wasm3_static` backend.
  - `AppController` no longer hard-disables WASM on non-Windows.

## Code Changes
1. Runtime backend model and factory
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmRuntimeFactory.h`
  - Added `RuntimeBackend::Wasm3Static`.
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmRuntimeFactory.cpp`
  - Added backend string mapping `wasm3_static`.
  - Added explicit `CreateRuntime(Wasm3Static)` path.
  - Default selection policy:
    - Windows: `dynamic_bridge` -> `null` fallback.
    - Non-Windows: `wasm3_static` -> `null` fallback.

2. Platform runtime factory split
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/PlatformWasmRuntimeFactory.h`
  - Added `CreateWasm3StaticRuntime`.
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/PlatformWasmRuntimeFactory.cpp`
  - Implemented `CreateWasm3StaticRuntime`:
    - Windows: unavailable (explicit error).
    - macOS/Linux: returns `Wasm3Runtime`.

3. New wasm3 runtime implementation
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/Wasm3Runtime.h`
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/Wasm3Runtime.cpp`
- Runtime behavior mirrors existing Windows bridge contract:
  - load wasm module from file
  - resolve `mfx_plugin_get_api_version`, `mfx_plugin_on_event`, optional `mfx_plugin_reset`
  - link host abort imports
  - enforce input/output scratch-memory bounds
  - propagate clear runtime errors

4. AppController WASM gating removal (core lane)
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.Wasm.cpp`
  - Removed compile-time non-Windows hard-disable logic.
  - Unified host initialization/config apply path across platforms.
  - Manifest load path now active on macOS core lane.

5. Build wiring (macOS core lane only)
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
  - Under `macos + MFX_ENABLE_POSIX_CORE_RUNTIME`:
    - enable C language
    - build `mfx_wasm3_core_runtime` static library from vendored wasm3 C sources
    - link `mfx_entry_runtime_common` with `mfx_wasm3_core_runtime`
    - include `Wasm3Runtime.cpp` in core lane sources
- Scope is intentionally limited to macOS core lane to avoid Windows/scaffold regression.

6. Regression contract hardening
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`
  - Added `/api/state` assertion for `wasm` section existence.
  - Added macOS assertion: `"runtime_backend":"wasm3_static"`.

## Validation
1. Build
- `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-macos-core-build-wasm3 -DMFX_PACKAGE_PLATFORM=macos -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON -DMFX_ENABLE_POSIX_CORE_RUNTIME=ON`
- `cmake --build /tmp/mfx-platform-macos-core-build-wasm3 --target mfx_entry_posix_host -j8`
- Result: pass.

2. Core lane smoke + contracts
- `./tools/platform/regression/run-posix-core-smoke.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build-wasm3`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build-wasm3`
- Result: pass.

3. Scaffold and cross-platform follow
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-linux-compile-gate.sh --build-dir /tmp/mfx-platform-linux-build --jobs 8`
- `pnpm --dir MFCMouseEffect/WebUIWorkspace run test:automation-platform`
- Result: pass.

## Risk Notes
- This phase only lands runtime bootstrap, not full WASM effect acceptance on macOS UI path.
- Next slice should focus on phase55b end-to-end plugin validation and fallback diagnostics closure.
