# Phase 53af - AppController Lifecycle Runtime Split

## Background
- `AppController.cpp` 仍承载生命周期主流程（`Start/Stop`）和核心状态编排，文件职责偏重。
- 生命周期代码同时影响四条能力链路（特效、WASM、键鼠指示、手势映射）的启动与退出，适合单独收口成独立编译单元。
- 需要在拆分时同步保持 POSIX 与 Windows 构建接线一致，避免平台漂移。

## Decision
- 将生命周期实现拆分为独立文件：
  - `AppController.Lifecycle.cpp`
- 保持行为不变，只做职责切分。
- 同步更新 CMake 与 Visual Studio 工程文件，保证 macOS/Linux/Windows 编译单元一致。

## Code Changes
1. 生命周期实现拆分
- 新增 `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.Lifecycle.cpp`
- 从 `AppController.cpp` 迁移实现：
  - `Start`
  - `Stop`
  - `CreateDispatchWindow`
  - `DestroyDispatchWindow`
- 新文件局部持有生命周期私有常量：
  - `kPlatformInvalidHandleError`

2. 主控制器文件瘦身
- 更新 `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
- 删除已迁移生命周期实现，仅保留运行时路由/效果编排等职责。
- 行数变化：
  - `AppController.cpp`: 797 -> 672

3. 跨平台构建接线同步
- 更新 `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
  - 加入 `AppController.Lifecycle.cpp`
- 更新 `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MFCMouseEffect.vcxproj`
  - 加入 `ClCompile` 条目
- 更新 `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MFCMouseEffect.vcxproj.filters`
  - 加入源文件过滤分组

## Behavior Compatibility
- 启动、降级、恢复、退出语义保持不变。
- API 路由、配置契约、诊断字段保持不变。
- 保持 Windows 车道无行为变更，仅补编译单元声明。

## Functional Ownership
- Category: `特效` + `WASM` + `键鼠指示` + `手势映射`（共享生命周期主路径）。
- Coverage: 启动阶段初始化与退出阶段释放，涵盖四大能力共用依赖。

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

2. `./tools/docs/doc-hygiene-check.sh --strict`
- Result: passed.
