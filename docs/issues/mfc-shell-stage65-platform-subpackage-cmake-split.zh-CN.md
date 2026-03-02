# Stage65 - 平台子包 CMake 拆分（windows/macos/linux/posix）

## 判定

`架构演进`：

1. Stage64 的 `Platform/CMakeLists.txt` 仍内联各平台源清单，平台边界在构建层面不够清晰。
2. 目标是对齐分包思路，让每个平台目录对自己的构建输入负责，根入口只做编排。

## 目标

1. 将 shell 包构建从“单文件清单”拆成“平台子目录 CMake”。
2. 引入 `posix` 共享构建单元，避免 macOS/Linux 重复维护共享源。
3. 保持主工程（VS `.slnx/.vcxproj`）行为不变。

## 变更摘要

### 1) 根入口改为编排器

更新：

- `MFCMouseEffect/Platform/CMakeLists.txt`

改动：

1. 根脚本仅负责：
   - 解析 `MFX_PACKAGE_PLATFORM`
   - 解析 host 支持
   - 条件加载 `posix` 共享包
   - 下钻 `windows/macos/linux` 子包
2. 新增 `MFX_ENABLE_CROSS_HOST_PACKAGES`（默认 `OFF`）：
   - 防止在非目标主机误构建不支持平台。
3. 当 `all` 模式遇到非 host 包时默认 `skip`，避免无效失败。

### 2) 每个平台目录新增独立 CMake

新增：

- `MFCMouseEffect/Platform/windows/CMakeLists.txt`
- `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- `MFCMouseEffect/Platform/linux/CMakeLists.txt`
- `MFCMouseEffect/Platform/posix/CMakeLists.txt`

说明：

1. `windows` 生成 `mfx_shell_windows`。
2. `macos` 生成 `mfx_shell_macos`，链接 `mfx_shell_posix_common`。
3. `linux` 生成 `mfx_shell_linux`，链接 `mfx_shell_posix_common`。
4. `posix` 生成共享静态库 `mfx_shell_posix_common`（含 `PosixBlockingEventLoop` + `Threads`）。

### 3) 删除旧式集中清单

删除：

- `MFCMouseEffect/Platform/cmake/PlatformShellSources.cmake`

目的：

1. 避免“集中式平台清单”与“平台分包”双轨并存。

### 4) 文档与忽略规则同步

更新：

- `MFCMouseEffect/Platform/windows/Shell/README.md`
- `MFCMouseEffect/Platform/macos/Shell/README.md`
- `MFCMouseEffect/Platform/linux/Shell/README.md`
- `.gitignore`

内容：

1. README 标注各平台源接线文件。
2. `.gitignore` 增加 `cmake_stage*/`、`cmake-stage*/`，避免临时 CMake 目录污染状态。

## 验证

1. 主工程回归：
   - `Release|x64` 构建通过（0 error / 0 warning）。
2. 边界检查：
   - `tools/architecture/Check-NonPlatformWin32Boundary.ps1` -> `OK`。
3. CMake 验证（Windows）：
   - `cmake -S MFCMouseEffect/Platform -B cmake_stage65_win_01 -G "Visual Studio 18 2026" -A x64 -DMFX_PACKAGE_PLATFORM=windows`
   - `cmake --build cmake_stage65_win_01 --config Release --target mfx_shell_windows`
   - 构建通过，产物 `mfx_shell_windows.lib`。

## 收益

1. 分平台构建边界进一步清晰，目录职责与构建职责一致。
2. `posix` 共享层在构建维度正式落地，macOS/Linux 复用路径明确。
3. 为后续 Stage66（入口/核心与分包目标联动）提供稳定基础。
