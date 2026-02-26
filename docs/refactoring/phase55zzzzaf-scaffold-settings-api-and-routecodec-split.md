# Phase 55zzzzaf - Scaffold Settings API and Route Codec Split

## Why
- `ScaffoldSettingsApi.cpp` mixed response/schema builders with state-patch parse/validation logic.
- `ScaffoldSettingsRouteConfig.cpp` mixed route policy with URL codec primitives (encode/decode/port parse).
- These mixed responsibilities increase change risk for scaffold lane maintenance.

## What Changed
- Split state patch parse/validation from API builders:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/ScaffoldSettingsApi.StatePatch.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/ScaffoldSettingsApi.cpp` now focuses on response/schema/state payload builders.
- Added dedicated route codec module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/ScaffoldSettingsRouteCodec.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/ScaffoldSettingsRouteCodec.cpp`
- Simplified route config module to route-level policy and parsing flow:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/posix/Shell/ScaffoldSettingsRouteConfig.cpp`
- Updated build wiring:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`

## Capability Mapping
- This change belongs to: scaffold/core shell infrastructure (shared support layer for `effects`, `input indicator`, `automation mapping`, `WASM`).
- No direct behavior change in four user-facing capability planes.

## Regression
- Command:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
  - `./tools/docs/doc-hygiene-check.sh --strict`
- Result:
  - Passed on macOS host (scaffold/core/automation/wasm/linux-gate/webui semantic checks all green).

## Risk
- Low.
- Responsibility split only; API/route contract semantics unchanged.
