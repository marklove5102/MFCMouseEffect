# Phase 53f: macOS Automation Scope UI Platform Semantics

## 判定先行
- 现象：
  - macOS 自动化 `Selected Apps` 选中项仍显示为 `*.exe`；
  - 相关按钮/占位文案仍是 Windows 语义（`exe`）。
- 判定：`Bug或回归`（跨平台语义未在 WebUI 层按平台切换）。

## 目标
1. macOS 下 app scope 选中项与扫描结果统一显示 `*.app` 语义。
2. macOS 下 app scope 相关文案与文件选择后缀改为 `app`。
3. 保持 Windows 既有 `exe` 行为不回归。

## 改动
1. macOS 应用扫描结果统一为 `.app`
- 文件：`MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanner.mm`
- 说明：
  - `ResolveProcessName(...)` 优先使用 bundle 名生成 `lowercase + ".app"`；
  - 避免返回无后缀可执行名，消除前端默认后缀猜测歧义。

2. 自动化模型按平台归一化进程名后缀
- 文件：`MFCMouseEffect/WebUIWorkspace/src/automation/model.js`
- 说明：
  - 增加平台识别（`windows/macos/linux`）与默认后缀策略；
  - `normalizeAppProcessName`、scope parse/serialize、validation/read/template 路径增加平台参数；
  - `normalizeAutomationPayload(...)` 输出 `platform`，供 UI 侧统一消费。

3. 应用目录列表归一化按平台处理
- 文件：`MFCMouseEffect/WebUIWorkspace/src/automation/app-catalog.js`
- 说明：
  - 目录项归一化与过滤时按平台后缀策略处理，mac 不再补成 `.exe`。

4. MappingPanel 平台化
- 文件：`MFCMouseEffect/WebUIWorkspace/src/automation/MappingPanel.svelte`
- 说明：
  - 新增 `platform` 入参；
  - 目录过滤/归一化时透传平台；
  - 文件选择 `accept` 改为平台动态：mac `.app`、windows `.exe`。

5. AutomationEditor 文案与数据流平台化
- 文件：`MFCMouseEffect/WebUIWorkspace/src/automation/AutomationEditor.svelte`
- 说明：
  - 保存 `runtimePlatform`（来自 schema `capabilities.platform`）；
  - scope parse/serialize/read/validate/template 全链路透传平台；
  - mac 下 app scope 文案切换为 app 语义（按钮、占位、错误提示）。

6. i18n 新增 mac 语义键
- 文件：`MFCMouseEffect/WebUI/i18n.js`
- 说明：
  - 新增 `*_macos` / `*_app` 相关键（中英文）；
  - 保持原 `exe` 键不变，供 Windows 继续使用。

7. 平台识别兜底（防 schema 缺失/别名误判）
- 文件：
  - `MFCMouseEffect/WebUIWorkspace/src/automation/platform.js`
  - `MFCMouseEffect/WebUIWorkspace/src/automation/model.js`
  - `MFCMouseEffect/WebUIWorkspace/src/automation/app-catalog.js`
  - `MFCMouseEffect/WebUIWorkspace/src/automation/MappingPanel.svelte`
- 说明：
  - 新增统一平台识别模块，支持 `macos/mac/darwin`、`windows/win*`、`linux`；
  - 当后端 `schema.capabilities.platform` 缺失或非标准值时，按浏览器环境自动兜底；
  - 避免 mac 环境被误判为 windows 导致 UI 继续显示 `exe` 文案。

## 验证
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
pnpm --dir MFCMouseEffect/WebUIWorkspace run build:automation
node MFCMouseEffect/WebUIWorkspace/scripts/copy-output.mjs
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- 结果：全部通过。

## 手工验收建议
1. macOS 设置页 -> 自动化 -> `Selected Apps` -> `Refresh app list`：
- 预期：列表与选中项显示 `*.app`。
2. 文案检查：
- 预期：按钮为“从文件选择 app”/“Pick app file”，占位示例为 `code.app`。
3. Windows 回归：
- 预期：仍保持 `exe` 语义与选择行为。

## 手工验收结果（2026-02-24）
- 结论：`通过`（用户实测反馈“测试没问题”）。
- 观测点：
  - macOS 自动化 scope 文案不再显示“从文件选择 exe”，已切换为 app 语义；
  - 本轮修复后的前端产物已全量重编译并同步到 `WebUI`，运行时生效。
