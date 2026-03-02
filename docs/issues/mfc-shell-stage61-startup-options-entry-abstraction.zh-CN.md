# Stage61 - 启动参数入口抽象（StartupOptions）

## 判定

`架构演进`：

1. `-mode background` 解析逻辑之前内嵌在 `Win32AppShell`，属于平台入口关注点，不应绑死在某个平台壳实现中。
2. `IPlatformAppShell::Initialize()` 无参数，导致入口层无法统一传递启动选项，后续 macOS/Linux 入口接线需要重复改接口。

## 目标

1. 将启动参数解析上移到平台入口工厂，形成统一的 `AppShellStartOptions` 输入。
2. 让 `IPlatformAppShell` 生命周期接口显式接收启动选项。
3. 保持 Windows 现有行为一致（`-mode background` 仍生效）。

## 变更摘要

### 1) 拆出独立启动选项模型

新增：

- `MFCMouseEffect/MouseFx/Core/Shell/AppShellStartOptions.h`

调整：

- `MFCMouseEffect/MouseFx/Core/Shell/AppShellCore.h`

说明：

1. `AppShellStartOptions` 从 `AppShellCore.h` 内联定义拆分为独立头文件。
2. 降低 `AppShellCore` 头文件耦合，便于入口层和平台层直接复用。

### 2) 平台入口接口改为显式接收 options

更新：

- `MFCMouseEffect/Platform/IPlatformAppShell.h`
- `MFCMouseEffect/Platform/PlatformAppShellFactory.cpp`
- `MFCMouseEffect/Platform/windows/Shell/Win32AppShell.h`
- `MFCMouseEffect/Platform/windows/Shell/Win32AppShell.cpp`

说明：

1. `IPlatformAppShell::Initialize` 签名改为：
   - `Initialize(const AppShellStartOptions& options)`
2. Windows/Posix/Null 壳实现全部对齐新签名。
3. `Win32AppShell` 移除内置命令行解析，改为只消费外部传入的 options。

### 3) 新增平台启动选项工厂并接入入口

新增：

- `MFCMouseEffect/Platform/PlatformStartupOptionsFactory.h`
- `MFCMouseEffect/Platform/PlatformStartupOptionsFactory.cpp`

更新：

- `MFCMouseEffect/MFCMouseEffect.cpp`

说明：

1. 新增 `CreatePlatformStartupOptions()`，负责平台入口参数解析。
2. 当前 Windows 下支持：
   - `-mode background` -> `showTrayIcon=false`
   - `-mode tray|normal` -> `showTrayIcon=true`
3. 入口改为：
   - 先取 `options`
   - 再调用 `app->Initialize(options)`。

### 4) 工程文件同步

更新：

- `MFCMouseEffect/MFCMouseEffect.vcxproj`
- `MFCMouseEffect/MFCMouseEffect.vcxproj.filters`

内容：

1. 新增 `AppShellStartOptions.h`、`PlatformStartupOptionsFactory.h/.cpp` 项目条目。

## 验证

1. `Release|x64` 构建通过（0 error / 0 warning）。
2. 架构边界检查通过：
   - `tools/architecture/Check-NonPlatformWin32Boundary.ps1`
   - 输出 `OK`。

## 收益

1. 启动参数成为入口层标准输入，平台壳只负责执行，不再自己解析命令行。
2. 后续接入 macOS/Linux 原生入口时，可以直接复用同一套 `AppShellStartOptions` 流程。
