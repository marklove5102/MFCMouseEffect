# Stage66 - CMake Presets 与平台构建说明

## 判定

`架构演进`：

1. Stage65 完成了平台子包 CMake 拆分，但使用方式仍依赖手写命令参数，协作门槛偏高。
2. 需要标准化入口（preset）与最小使用文档，确保团队可重复执行分包构建。

## 目标

1. 提供稳定的 preset 入口，覆盖 host-auto 与 windows/macOS/linux 包构建。
2. 在 Platform 层补齐构建说明文档，明确目标与结构。
3. 避免本地 CMake 目录污染工作区状态。

## 变更摘要

### 1) 新增 CMakePresets

新增：

- `MFCMouseEffect/Platform/CMakePresets.json`

内容：

1. `configurePresets`：
   - `host-auto-vs2026`
   - `windows-shell-vs2026`
   - `macos-shell-vs2026`
   - `linux-shell-vs2026`
2. `buildPresets`：
   - `build-host-auto-release`
   - `build-windows-shell-release`

### 2) 新增 Platform 构建总览文档

新增：

- `MFCMouseEffect/Platform/README.md`

内容：

1. 说明平台包目标、目录结构、preset 名称与调用示例。
2. 强调主应用链接仍走 `MFCMouseEffect.slnx`。

### 3) 忽略 CMake 产物目录

更新：

- `.gitignore`

内容：

1. 增加 `cmake_build/` 忽略项，配合 presets 的 `binaryDir` 使用。

## 验证

1. `cmake --list-presets` 可识别 `Platform/CMakePresets.json` 中的 preset。
2. `windows-shell-vs2026` 相关配置/构建流程可用（结合 Stage65 的 Windows 包验证）。
3. 主工程 `Release|x64` 构建保持通过（0 error / 0 warning）。

## 收益

1. 分包构建从“手写命令”提升为“可复用 preset 流程”。
2. 降低团队成员进入跨平台包构建路径的学习成本。
3. 为 Stage67（非 Windows 平台构建链路接线）提供统一入口。
