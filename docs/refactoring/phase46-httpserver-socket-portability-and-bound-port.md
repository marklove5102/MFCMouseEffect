# Phase 46: HttpServer Socket Portability and Bound-Port Support

## Top Decisions (Important)
- Keep `HttpServer` API backward compatible (`StartLoopback` unchanged).
- Add explicit bound-port startup for scaffold runtime (`StartLoopbackOnPort`).
- Make socket transport/protocol code host-portable (Windows + POSIX) without platform-specific callers.

## 判定先行
- 判定：`Bug或回归风险修复`
- 依据：
  1. `Stop()` 阶段对 `listenSock_` 的提前写回会和 `Run()` 线程读取形成并发竞态窗口。
  2. `Content-Length` 请求体在连接提前关闭时可能被当作成功请求继续处理。
  3. 固定端口需求已在 POSIX scaffold 场景出现，原接口只支持临时端口。

## Code Changes
1. `MouseFx/Server/HttpServer.h`
   - introduced `SocketHandle` alias
   - added `StartLoopbackOnPort(uint16_t port, Handler handler)`
   - switched internal socket parameter/field to `SocketHandle`
2. `MouseFx/Server/HttpServer.cpp`
   - added native-socket adapter helpers for Windows/POSIX
   - added `StartLoopbackOnPort` implementation (fixed port or ephemeral)
   - `StartLoopback` now delegates to `StartLoopbackOnPort(0, ...)`
   - `Stop()` now closes listen socket without early handle overwrite; handle reset after join
   - added startup failure cleanup on thread creation failure
3. `MouseFx/Server/HttpServer.Protocol.cpp`
   - added protocol-local trim/lower helpers (remove dependency on external string utils)
   - switched send/recv helpers to `SocketHandle` + native-socket adapter
   - send path now supports large body chunking safely
   - request parse now rejects incomplete body when `Content-Length` is not fully received

## Compatibility
- Existing callers using `StartLoopback(...)` remain valid.
- POSIX scaffold can bind deterministic port via `StartLoopbackOnPort(...)`.
- Windows code path preserved via winsock branch; POSIX path enabled for macOS/Linux.

## Validation
- Executed:
  - `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
  - `cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-cross-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON`
  - `cmake --build /tmp/mfx-platform-linux-cross-build --target mfx_shell_linux mfx_entry_posix -j8`
- Result:
  - macOS regression passed
  - linux cross-host scaffolding build passed
