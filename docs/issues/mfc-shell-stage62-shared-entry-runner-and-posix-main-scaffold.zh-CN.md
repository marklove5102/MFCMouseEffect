# Stage62 - 共享入口 Runner 与 Posix main 脚手架

## 判定

`架构演进`：

1. `MFCMouseEffect.cpp` 直接承载“创建 shell + 启动参数 + 生命周期”流程，不利于多平台入口复用。
2. 非 Windows 未来会使用 `main`，如果没有共享入口 runner，会再次复制一套启动逻辑。

## 目标

1. 抽出跨平台可复用的入口执行器（Entry Runner）。
2. 让 Windows `wWinMain` 与未来 Posix `main` 走同一条启动链路。
3. 保持现有 Windows 行为不变。

## 变更摘要

### 1) 新增共享入口执行器

新增：

- `MFCMouseEffect/Platform/PlatformEntryRunner.h`
- `MFCMouseEffect/Platform/PlatformEntryRunner.cpp`

职责：

1. 统一执行入口流程：
   - 解析 `CreatePlatformStartupOptions()`
   - 创建 `CreatePlatformAppShell()`
   - 调用 `Initialize(options)` / `RunMessageLoop()` / `Shutdown()`
2. 返回进程退出码。

### 2) Windows 入口改为调用共享 Runner

更新：

- `MFCMouseEffect/MFCMouseEffect.cpp`

改动：

1. `wWinMain` 从直接编排生命周期改为：
   - `return mousefx::platform::RunPlatformEntry();`

### 3) 增加 Posix main 脚手架

新增：

- `MFCMouseEffect/Platform/posix/Entry/PosixMain.cpp`

说明：

1. 提供 `main` 脚手架并调用 `RunPlatformEntry()`。
2. 当前为后续 macOS/Linux 构建系统接线预留，Windows 工程未纳入该文件。

### 4) 工程文件同步

更新：

- `MFCMouseEffect/MFCMouseEffect.vcxproj`
- `MFCMouseEffect/MFCMouseEffect.vcxproj.filters`

内容：

1. 新增 `PlatformEntryRunner.h/.cpp` 条目。

## 验证

1. `Release|x64` 构建通过（0 error / 0 warning）。
2. 边界检查通过：
   - `tools/architecture/Check-NonPlatformWin32Boundary.ps1` -> `OK`。

## 收益

1. 入口逻辑单点化，减少平台入口重复代码。
2. 为 macOS/Linux 入口接线提供稳定复用点，降低后续 Stage 的改动面。
