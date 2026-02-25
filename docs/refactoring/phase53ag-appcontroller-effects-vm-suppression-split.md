# Phase 53ag - AppController Effects and VM Suppression Split

## Background
- `AppController.cpp` 仍混合了输入路由与效果编排（effect set/apply/theme）以及 VM 前台抑制逻辑。
- 这些逻辑变更频率高，且与 macOS 主线的权限/自动化回归链路耦合，继续堆在主文件会增加评审与回归风险。

## Decision
- 将“效果编排 + VM 抑制”职责拆分到独立实现文件：
  - `AppController.Effects.cpp`
- 保持行为不变，只做职责解耦。
- 同步更新 POSIX 与 Windows 工程接线，避免平台编译单元漂移。

## Code Changes
1. 新增效果编排实现单元
- Added `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.Effects.cpp`
- 包含以下实现迁移：
  - effect 管理：`CreateEffect/SetEffect/ClearEffect/ApplyConfiguredEffects/NormalizeActiveEffectTypes`
  - active type 映射：`ActiveTypeForCategory/MutableActiveTypeForCategory/SetActiveEffectType`
  - 主题与 hold 运行时：`SetTheme/SetHoldFollowMode/SetHoldPresenterBackend`
  - VM 抑制链路：`UpdateVmSuppressionState/ApplyVmSuppression/SuspendEffectsForVm/ResumeEffectsAfterVm`
  - GPU fallback 诊断：`NotifyGpuFallbackIfNeeded/WriteGpuRouteStatusSnapshot`

2. 主控制器瘦身
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
- 删除已迁移实现，仅保留输入分发与编排入口职责。
- 行数变化：
  - `AppController.cpp`: 672 -> 387

3. 跨平台工程接线
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/CMakeLists.txt`
  - 加入 `AppController.Effects.cpp`
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MFCMouseEffect.vcxproj`
  - 加入 `ClCompile` 条目
- Updated `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MFCMouseEffect.vcxproj.filters`
  - 加入源文件过滤分组

## Behavior Compatibility
- 事件路由、权限降级/恢复、效果显示、VM 抑制行为保持不变。
- API 路径与设置契约保持不变。
- Windows 车道仅接线新增，不改运行语义。

## Functional Ownership
- Category: `特效` + `键鼠指示` + `手势映射`（共享 effect/VM 抑制路径）。
- Coverage: effect 管理、主题应用、VM 前台抑制下的 effect/indicator/automation 状态收敛。

## Verification
1. `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- Result: passed.

2. `./tools/docs/doc-hygiene-check.sh --strict`
- Result: passed.
