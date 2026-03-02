# Stage60 - 非 Windows AppShell 接线与 WASM 工厂平台守卫

## 判定

`架构债务`：

1. `PlatformAppShellFactory` 在非 Windows 仍返回 `NullPlatformAppShell`，导致 macOS/Linux 入口无法进入真实 `AppShellCore` 生命周期。
2. `PlatformWasmRuntimeFactory` 无条件依赖 `Platform/windows/Wasm/Win32DllWasmRuntime.h`，阻断非 Windows 编译与分包。

## 目标

1. 非 Windows（macOS/Linux）入口从空壳切换为真实 shell 生命周期。
2. WASM 动态桥工厂按平台编译，消除非 Windows 对 Win32 头/实现的直接依赖。
3. 保持 Windows 行为不变。

## 变更摘要

### 1) AppShell 工厂非 Windows 从 Null 改为 Core-backed

更新文件：

- `MFCMouseEffect/Platform/PlatformAppShellFactory.cpp`

改动：

1. 新增 `PosixPlatformAppShell`（`__APPLE__`/`__linux__`）：
   - 内部持有 `AppShellCore`。
   - 初始化时合并默认 `ShellPlatformServices`（tray/settings/single-instance/event-loop/notifier 等）。
   - 生命周期转发：`Initialize/RunMessageLoop/Shutdown`。
2. `CreatePlatformAppShell` 在 macOS/Linux 返回 `PosixPlatformAppShell`。
3. 仅保留未知平台的 `NullPlatformAppShell` 兜底。

### 2) WASM 动态桥工厂增加平台守卫

更新文件：

- `MFCMouseEffect/Platform/PlatformWasmRuntimeFactory.cpp`

改动：

1. `Win32DllWasmRuntime` include 改为 `_WIN32` 条件编译。
2. `CreateDynamicBridgeWasmRuntime`：
   - Windows：保持原逻辑（创建并初始化 Win32 动态桥运行时）。
   - 非 Windows：返回 `nullptr` 并写出平台不支持错误信息。

## 行为说明

1. Windows 入口和 WASM 行为保持不变。
2. macOS/Linux 现在可以通过 `PlatformAppShellFactory` 进入真实 `AppShellCore` 流程，不再是立即失败的空实现。
3. 非 Windows WASM 动态桥仍按设计回退到 `NullWasmRuntime`（由 Core 工厂兜底），但已去除 Win32 头文件硬耦合。

## 验证

1. `Release|x64` 构建通过（Windows）。
2. 代码路径检查：
   - 非 Windows `CreatePlatformAppShell` 不再返回 `NullPlatformAppShell`。
   - 非 Windows `CreateDynamicBridgeWasmRuntime` 不再包含 Win32 依赖。

## 风险与后续

1. 当前入口仍是 `wWinMain`，真正的 macOS/Linux 可执行入口（`main` + 对应构建系统）属于后续阶段。
2. 非 Windows WASM 动态桥目前是“明确不支持并回退”，后续如要启用需补充对应平台 runtime backend。
