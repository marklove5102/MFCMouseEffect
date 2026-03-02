# Phase 53b: Automation `app_scope` Cross-Platform Normalization

## 判定先行
- 现状：`app_scope` 归一化与运行时匹配逻辑分散在 `EffectConfig.Internal` 与 `InputAutomationEngine` 两处，且匹配是严格字符串相等。
- 判定：`Bug或回归`（跨平台共享配置时，`process:code.exe` / `process:safari.app` / `process:code` 在 macOS 与 Linux 上命中不稳定）。

## 目标
1. 将 `app_scope` 语义归一到单一 helper，降低重复实现与漂移风险。
2. 保持 Windows 既有默认行为，同时补齐 `.exe/.app/无后缀` 兼容匹配。
3. 不破坏 scaffold 车道和 Linux 编译跟随。

## 改动
1. 新增 `app_scope` 公共语义 helper（header-only）
- 文件：`MFCMouseEffect/MouseFx/Core/Automation/AppScopeUtils.h`
- 内容：
  - 统一 `all/global/*` 与 `process:*` 解析。
  - 统一进程名清洗：去路径、去成对引号、ASCII 小写、空白裁剪。
  - Windows 维持“无后缀自动补 `.exe`”策略。
  - 引入进程别名集合匹配（`.exe/.app/无后缀`）用于跨平台兼容。

2. 配置归一化改为复用 helper
- 文件：`MFCMouseEffect/MouseFx/Core/Config/EffectConfig.Internal.cpp`
- 内容：
  - 删除本地重复的进程名/`app_scope` 归一化实现。
  - `NormalizeAutomationAppScopeToken` 改为调用 `automation_scope::NormalizeScopeToken`。
  - `appScopes` 去重时增加语义级去重（`process:code` 与 `process:code.exe` 视为同一目标）。

3. 运行时匹配改为复用 helper
- 文件：`MFCMouseEffect/MouseFx/Core/Automation/InputAutomationEngine.cpp`
- 内容：
  - `AppScopeMatches` 改为使用公共归一化与别名匹配，不再 strict-equal。
  - `AppScopeSpecificity` 改为同源解析，避免配置与运行时语义偏差。

4. 契约注释更新
- 文件：`MFCMouseEffect/MouseFx/Core/Config/EffectConfig.h`
- 内容：`app_scope` 注释从 `exe_name` 扩展为跨平台 `process_name` 示例。

## 验证
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- 结果：全部 passed。

## 风险与边界
1. 兼容匹配仅对 `.exe/.app/无后缀` 做等价处理；带其他后缀（如 `.bin`）仍按严格名称匹配，避免过度放宽。
2. 该阶段不改注入策略与权限链路，仅改 `app_scope` 归一化/匹配语义。

## 手工验收建议（Phase 53 收口）
1. 在 macOS 设置映射：`left_click -> Cmd+C`，作用域设为 `process:code.exe`；前台 VS Code 时应可触发。
2. 将作用域改为 `process:code` 与 `process:code.app` 复测；预期行为一致。
3. 保持 `all` 作用域映射，验证其仍优先作为全局兜底不受影响。
