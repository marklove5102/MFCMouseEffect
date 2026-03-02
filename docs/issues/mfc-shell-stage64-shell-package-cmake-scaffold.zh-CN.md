# Stage64 - Shell 分包 CMake 脚手架

## 判定

`架构演进`：

1. 当前跨平台目录已经按 `windows/macos/linux/posix` 拆分，但缺少独立的分包构建入口，跨平台产物编排仍主要依赖 VS 工程。
2. 目标是对齐 Flutter 式“分平台包”思路，需要先有平台包级别的目标定义，不直接改动主程序链接路径。

## 目标

1. 增加独立于主工程的 Shell 分包构建脚手架。
2. 以平台为粒度提供 `windows/macos/linux` 的 shell 包目标。
3. 不改变现有 Windows 主工程（`.slnx/.vcxproj`）构建链路。

## 变更摘要

### 1) 新增 Shell 源清单模块

新增：

- `MFCMouseEffect/Platform/cmake/PlatformShellSources.cmake`

职责：

1. 按平台集中维护 shell 包源文件集合：
   - `windows`
   - `macos`
   - `linux`
2. 与主 `CMakeLists` 分离，避免单文件过大。

### 2) 新增平台分包主 CMake 入口

新增：

- `MFCMouseEffect/Platform/CMakeLists.txt`

能力：

1. `MFX_PACKAGE_PLATFORM` 选择包：
   - `auto|windows|macos|linux|all`
2. 统一 contracts 目标：
   - `mfx_shell_contracts`（接口头目录 + C++17）
3. 生成平台包目标：
   - `mfx_shell_windows`
   - `mfx_shell_macos`
   - `mfx_shell_linux`

说明：

1. 这是 shell 层静态库脚手架，主程序链接仍走现有 VS 工程。
2. 对 posix 包自动链接 `Threads::Threads`。

### 3) 平台 README 同步

新增：

- `MFCMouseEffect/Platform/windows/Shell/README.md`

更新：

- `MFCMouseEffect/Platform/macos/Shell/README.md`
- `MFCMouseEffect/Platform/linux/Shell/README.md`

内容：

1. 统一标注 CMake 分包目标名称，明确分包定位。

### 4) CMake 输出目录忽略

更新：

- `.gitignore`

内容：

1. 新增 `out/` 忽略规则，避免本地 CMake 试验目录污染工作区状态。

## 验证

1. `Release|x64`（VS 主工程）构建通过：`0 error / 0 warning`。
2. 边界检查通过：
   - `tools/architecture/Check-NonPlatformWin32Boundary.ps1` -> `OK`。
3. CMake 验证：
   - 使用 VS 自带 `cmake.exe` 可识别脚本；
   - 当前机上配置阶段触发 `VCTargetsPath.lastbuildstate` 写入权限拒绝（MSBuild 侧环境问题），因此未完成 CMake 目标编译验证。

## 收益

1. 平台目录从“代码分层”升级到“可分包编排入口”。
2. 为后续 macOS/Linux 独立构建与产物拆分提供直接落地点。
3. 保持现有 Windows 主线零行为变化，降低迁移风险。
