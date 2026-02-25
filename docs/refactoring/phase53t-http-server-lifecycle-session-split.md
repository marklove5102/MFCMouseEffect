# Phase 53t - HttpServer Lifecycle and Session Split

## Background
- `HttpServer.cpp` previously mixed lifecycle control (`start/stop/accept loop`) and client request session handling in one file.
- This increased coupling for future platform/runtime evolution and made targeted review harder.

## Decision
- Keep HTTP behavior and route contracts unchanged.
- Split `HttpServer` internals into focused modules:
  - lifecycle module for socket startup/shutdown and accept loop
  - client-session module for request parse/handler dispatch/error mapping
  - protocol module remains dedicated to parse/response encoding
- Keep `HttpServer.cpp` as constructor/destructor entry only.

## Code Changes
1. Client-session module
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/HttpServer.ClientSession.cpp`
- Moved `HandleClient(...)` request lifecycle:
  - parse failure -> `400 bad request`
  - handler exception -> `500 internal error` mapping
  - success -> `SendResponse(...)`

2. Lifecycle module wiring
- Kept lifecycle implementation in `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/HttpServer.Lifecycle.cpp`
- Covers:
  - loopback socket bind/listen
  - worker thread run loop
  - graceful stop/cleanup

3. Entry simplification
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/HttpServer.cpp`
- Now only retains ctor/dtor and `Stop()` call in dtor.

4. Build wiring
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
- Added:
  - `HttpServer.ClientSession.cpp`
  - `HttpServer.Lifecycle.cpp`

## Behavior Compatibility
- HTTP request/response protocol semantics unchanged.
- Scaffold/core lane endpoints and token checks unchanged.
- This phase is structure-only refactor.

## Functional Ownership
- Category: `共用控制面`
- Coverage: `特效 / WASM / 键鼠指示 / 手势映射` (shared WebSettings transport layer)

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.
