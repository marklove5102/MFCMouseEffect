# Phase 53a: macOS Automation System Services Bootstrap

## 判定先行
- 现状: `InputAutomationEngine` 在 macOS 仍绑定 `NullKeyboardInjector` 与 `NullForegroundProcessService`。
- 判定: `Bug或回归`（Phase 53 能力缺口，动作映射在 mac 上无完整执行链路）。

## 目标
1. 为 macOS 提供最小可用键盘注入实现，打通动作映射的执行出口。
2. 为 macOS 提供前台进程名解析实现，打通 `appScopes` 过滤与 `/api/automation/active-process` 数据来源。
3. 不改变 Windows 既有行为，不破坏 Linux 编译跟随。

## 改动
1. 新增 macOS 键盘注入服务
- 文件: `MFCMouseEffect/Platform/macos/System/MacosKeyboardInjector.h`
- 文件: `MFCMouseEffect/Platform/macos/System/MacosKeyboardInjector.mm`
- 说明:
  - 复用 `ParseKeyChord` 解析跨平台快捷键字符串；
  - 将 `Meta/Cmd` 统一语义映射到 macOS Command 键；
  - 支持常见字母/数字/F 键/方向键/Tab/Enter/Escape/PageUp/PageDown 等基础注入。

2. 新增 macOS 前台进程解析服务
- 文件: `MFCMouseEffect/Platform/macos/System/MacosForegroundProcessService.h`
- 文件: `MFCMouseEffect/Platform/macos/System/MacosForegroundProcessService.mm`
- 说明:
  - 通过 `NSWorkspace` 读取 frontmost app，解析当前进程基名；
  - 当前台 app 不可用时，回退到 `currentApplication -> NSProcessInfo.processName -> "unknown"`，避免 `/api/automation/active-process` 空串；
  - 保持与 Windows 一致的 200ms 级缓存策略，降低轮询开销。

3. 工厂与构建接线
- 文件: `MFCMouseEffect/Platform/PlatformInputServicesFactory.cpp`
- 文件: `MFCMouseEffect/Platform/PlatformSystemServicesFactory.cpp`
- 文件: `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- 说明:
  - mac 平台改为创建 `MacosKeyboardInjector` 与 `MacosForegroundProcessService`；
  - 将新增 macOS system 源文件纳入 `mfx_shell_macos` 构建。

4. schema 能力声明修正（避免 UI/契约误判）
- 文件: `MFCMouseEffect/MouseFx/Server/SettingsSchemaBuilder.cpp`
- 说明:
  - `capabilities.input.keyboard_injector` 从“仅 Windows 为 true”修正为“Windows/macOS 为 true”；
  - 与实际工厂接线保持一致，避免 mac 已支持但 schema 声称不支持。

5. 注入链路测试探针（测试态开关）
- 文件: `MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- 文件: `MFCMouseEffect/MouseFx/Core/Control/AppController.h`
- 文件: `MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
- 文件: `MFCMouseEffect/Platform/macos/System/MacosKeyboardInjector.mm`
- 说明:
  - 新增测试态 API：`POST /api/automation/test-inject-shortcut`（门禁 `MFX_ENABLE_AUTOMATION_INJECTION_TEST_API=1`）；
  - 新增 mac 注入器 dry-run 开关：`MFX_TEST_KEYBOARD_INJECTOR_DRY_RUN=1`；
  - dry-run 仍执行 chord 解析与键码映射校验，仅跳过实际 `CGEventPost`，确保契约回归可稳定执行且不污染系统输入。

## 验证
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- 结果: passed

## 脚本收口（2026-02-24）
1. core automation contract 增加 macOS 断言：
- `/api/automation/active-process` 返回 `process` 必须为非空字符串。
- `/api/schema` 的 `capabilities.input.keyboard_injector` 必须为 `true`。
- `POST /api/automation/test-inject-shortcut` 使用 `{"keys":"Cmd+C"}` 必须返回 `accepted=true`（dry-run 模式）。
2. 断言位置：
- `tools/platform/regression/lib/core_http.sh`
3. 结论：
- `active-process` 暴露、schema 能力声明、注入链路探针均已脚本化收口；真实 OS 级注入仍保留手工 E2E。

## 手工验收建议（最小）
1. 打开设置页自动化区域，配置 `left_click -> Cmd+C`（或任意 Cmd 组合）。
2. 在可编辑输入框聚焦状态触发映射。
- 预期: 快捷键注入生效。
3. 调用 `/api/automation/active-process`。
- 预期: `process` 字段返回非空进程基名（前台不可取时为回退值）。
