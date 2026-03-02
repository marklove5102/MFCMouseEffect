# Stage63 - 入口参数管线化与 Posix argv 接线

## 判定

`架构演进`：

1. `CreatePlatformStartupOptions()` 仅支持“平台自行读取命令行”，不支持从入口显式注入参数，不利于 `main(argc, argv)` 场景复用。
2. `PosixMain.cpp` 虽已存在，但没有把 `argv` 传入启动链路，入口参数在非 Windows 路径不可用。
3. `pch.h` 无条件包含 Win32 头，会成为非 Windows 编译阻塞点。

## 目标

1. 建立统一的入口参数模型，支持平台入口向 Runner 显式传参。
2. 打通 `Posix main -> PlatformEntryRunner -> StartupOptions` 的参数链路。
3. 让预编译头对平台感知，减少非 Windows 构建阻塞。

## 变更摘要

### 1) 新增入口参数模型

新增：

- `MFCMouseEffect/Platform/PlatformEntryArgs.h`

说明：

1. 定义 `PlatformEntryArgs`，目前包含 `argvUtf8`。
2. 作为平台入口与启动选项工厂之间的标准参数载体。

### 2) 入口 Runner 支持显式参数注入

更新：

- `MFCMouseEffect/Platform/PlatformEntryRunner.h`
- `MFCMouseEffect/Platform/PlatformEntryRunner.cpp`

改动：

1. 新增重载：
   - `RunPlatformEntry(const PlatformEntryArgs& entryArgs)`
2. 内部抽取 `RunPlatformEntryWithOptions(...)`，统一生命周期执行。
3. 原无参 `RunPlatformEntry()` 保留，兼容 Windows 现有入口。

### 3) StartupOptions 工厂支持两种输入来源

更新：

- `MFCMouseEffect/Platform/PlatformStartupOptionsFactory.h`
- `MFCMouseEffect/Platform/PlatformStartupOptionsFactory.cpp`

改动：

1. 新增重载：
   - `CreatePlatformStartupOptions(const PlatformEntryArgs& entryArgs)`
2. 新增统一参数解析逻辑（ASCII 忽略大小写）：
   - `-mode background`
   - `--mode background`
   - `-mode=background`
   - `--mode=background`
   - `tray/normal` 对应显示托盘
3. Windows 无参路径仍通过 `CommandLineToArgvW` 获取参数，再转换 UTF-8 走同一解析器。

### 4) Posix main 真实透传 argv

更新：

- `MFCMouseEffect/Platform/posix/Entry/PosixMain.cpp`

改动：

1. 将 `argc/argv` 填充到 `PlatformEntryArgs`。
2. 调用 `RunPlatformEntry(entryArgs)`，形成完整参数通路。

### 5) PCH 平台条件化

更新：

- `MFCMouseEffect/pch.h`

改动：

1. `Platform/windows/Common/Win32Base.h` 改为 `_WIN32` 条件包含。
2. 避免非 Windows 预编译头直接拉入 `windows.h`。

### 6) 工程文件同步

更新：

- `MFCMouseEffect/MFCMouseEffect.vcxproj`
- `MFCMouseEffect/MFCMouseEffect.vcxproj.filters`

内容：

1. 增加 `PlatformEntryArgs.h` 条目。

## 验证

1. `Release|x64` 构建通过（0 error / 0 warning）。
2. 边界检查通过：
   - `tools/architecture/Check-NonPlatformWin32Boundary.ps1` -> `OK`。

## 收益

1. 启动参数从“平台内部读取”升级为“可注入、可复用”的入口管线。
2. Posix 入口具备真实参数能力，后续接构建系统后可直接启用。
3. 进一步降低非 Windows 编译前置阻塞，跨平台基座更稳。
