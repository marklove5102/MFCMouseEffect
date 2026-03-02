# Stage67 - 分包构建补齐入口运行时目标

## 判定

`架构演进`：

1. Stage65/66 已能构建 shell 包，但入口链路（`RunPlatformEntry` + 启动参数 + AppShell 工厂）尚未进入分包目标。
2. 需要在 CMake 层补齐入口运行时目标，避免 shell 包与入口链路脱节。

## 目标

1. 新增可复用的入口运行时目标 `mfx_entry_runtime_common`。
2. 增加平台入口目标：
   - Windows：`mfx_entry_windows`
   - Posix：`mfx_entry_posix`（按 macOS/Linux 包启用）
3. 通过 preset 提供入口目标构建入口。

## 变更摘要

### 1) 根 CMake 增加入口运行时目标

更新：

- `MFCMouseEffect/Platform/CMakeLists.txt`

改动：

1. 新增开关：
   - `MFX_ENABLE_ENTRY_RUNTIME_TARGETS`（默认 `ON`）
2. 新增目标：
   - `mfx_entry_runtime_common`
     - `PlatformEntryRunner.cpp`
     - `PlatformStartupOptionsFactory.cpp`
     - `PlatformAppShellFactory.cpp`
     - `PlatformShellServicesFactory.cpp`
3. 入口目标按已配置的 shell 包自动链接对应 `mfx_shell_*` 目标。

### 2) 新增平台入口目标

更新：

- `MFCMouseEffect/Platform/CMakeLists.txt`

改动：

1. Windows 包存在时创建 `mfx_entry_windows`（源：`MFCMouseEffect.cpp`）。
2. macOS/Linux 包存在时创建 `mfx_entry_posix`（源：`Platform/posix/Entry/PosixMain.cpp`）。
3. 两者统一链接 `mfx_entry_runtime_common`。

### 3) Preset 与文档同步

更新：

- `MFCMouseEffect/Platform/CMakePresets.json`
- `MFCMouseEffect/Platform/README.md`

内容：

1. 新增 build preset：
   - `build-windows-entry-release`
2. README 增加入口运行时目标说明与构建示例。

## 验证

1. `cmake --preset windows-shell-vs2026` 配置通过。
2. `cmake --build --preset build-windows-shell-release` 通过。
3. `cmake --build --preset build-windows-entry-release` 通过（包含 `mfx_entry_runtime_common` 与 `mfx_entry_windows`）。
4. Windows 主工程 `Release|x64` 保持通过（0 error / 0 warning）。
5. 架构边界检查通过：
   - `tools/architecture/Check-NonPlatformWin32Boundary.ps1` -> `OK`。

## 收益

1. 分包体系从“仅 shell 组件”扩展到“入口 + 运行时 + shell”闭环。
2. 后续接入 macOS/Linux 可执行入口时，可直接复用 `mfx_entry_runtime_common` 与 `mfx_entry_posix`。
3. 跨平台分包路径更接近最终可交付结构。
