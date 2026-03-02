# Phase 53c: POSIX Core Settings WebUI Rewire

## 判定先行
- 现状：POSIX core 车道设置入口仍指向硬编码 `http://127.0.0.1:9527/?token=scaffold`，但 core 车道未启动 scaffold settings server。
- 判定：`Bug或回归`（托盘设置入口不可用，手工 `open` 固定 URL 也不可用）。

## 目标
1. 让 POSIX core 车道设置入口复用正式 `WebSettingsServer`（与 Windows 一致的 Svelte WebUI/API 链路）。
2. 修复 macOS 托盘状态项宽度异常（`MFX` 竖排显示）。
3. 保持 scaffold 默认车道行为不变，Linux 编译跟随继续通过。

## 改动
1. POSIX core shell 接入 `WebSettingsServer`
- 文件：`MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.h`
- 文件：`MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.cpp`
- 说明：
  - 删除 core 车道硬编码 settings URL 打开路径；
  - `OpenSettingsFromShell()` 改为按需启动 `WebSettingsServer` + `RotateToken()` + `Url()`；
  - 增加 event-loop `PostTask` 调度，保证托盘回调与壳层动作在同一调度域；
  - shell 关闭时同步停止 `WebSettingsServer`，避免悬挂服务。

2. WebSettingsServer 增强 WebUI 目录解析（优先复用现有 Svelte 产物）
- 文件：`MFCMouseEffect/MouseFx/Server/WebSettingsServer.cpp`
- 说明：
  - 新增多候选目录查找：
    - `MFX_WEBUI_DIR`（显式覆盖）
    - `exeDir/webui`
    - `cwd/MFCMouseEffect/WebUI`、`cwd/WebUI`、`cwd/../MFCMouseEffect/WebUI`
    - 源码树推断目录（基于 `__FILE__`）
  - 修复路径拼接中 Windows 反斜杠硬编码问题。

3. macOS 托盘状态项显示修正
- 文件：`MFCMouseEffect/Platform/macos/Shell/MacosTrayService.mm`
- 说明：状态项宽度从 `NSSquareStatusItemLength` 调整为 `NSVariableStatusItemLength`，避免 `MFX` 竖排/截断。

4. macOS 事件循环桥接修正（托盘点击链路）
- 文件：`MFCMouseEffect/Platform/macos/Shell/MacosEventLoopService.cpp`
- 文件：`MFCMouseEffect/Platform/macos/Shell/MacosEventLoopBridge.h`
- 文件：`MFCMouseEffect/Platform/macos/Shell/MacosEventLoopBridge.mm`
- 文件：`MFCMouseEffect/Platform/macos/CMakeLists.txt`
- 说明：
  - 事件循环从 `CFRunLoopRun` 切换为 `NSApp run` 驱动，避免仅跑 CF 默认模式导致状态栏点击事件无法进入 AppKit 菜单链路；
  - 保留既有 `PostTask` 任务队列与退出语义，通过桥接在 `RequestExit` 时调用 `NSApp stop` + wake event。

5. mac core 车道补齐构建链接依赖
- 文件：`MFCMouseEffect/Platform/CMakeLists.txt`
- 说明：
  - 为 core 车道补齐 `WebSettingsServer` 及其依赖平台 facade/registry 源；
  - 保持 Linux scaffold 编译路径不变。

6. 非 Windows 的 WASM catalog/import/export API 显式降级
- 文件：`MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- 说明：
  - `catalog/import/export` 在非 Windows 返回明确 `not_supported` 错误，避免 POSIX core 误链到 Windows-only 传输实现；
  - 不影响 M1 主链路（输入/指示/映射）。

## 验证
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
pkill -f mfx_entry_posix_host || true
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- 结果：全部 passed。

## 脚本收口（2026-02-24）
1. core automation contract 新增 WebUI 资源断言：
- `settings_url` 根页面 `GET` 必须为 `200`
- `GET /settings-shell.svelte.js?token=<token>` 必须为 `200`
2. core automation contract 新增 settings launcher 调用探针（测试态，不真实拉起浏览器）：
- `MFX_CORE_WEB_SETTINGS_LAUNCH_PROBE_FILE` 记录 `url/opened`，断言 `opened=1`
- `MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE` 记录 `command/url`，断言命令与 token 化 URL 一致
3. scaffold smoke 新增 tray settings 动作链路断言（测试态自动触发）：
- `mfx_shell_macos_tray_smoke` 在 `MFX_TEST_TRAY_SMOKE_EXPECT_SETTINGS_ACTION=1` 下自动触发托盘 `Settings` 动作；
- `MFX_TEST_SETTINGS_LAUNCH_CAPTURE_FILE` 记录 `command/url`，断言 `command=open` 且 URL 等于测试输入。
4. 断言位置：
- `tools/platform/regression/lib/core_http.sh`
- `tools/platform/regression/lib/smoke.sh`
5. 结论：
- core 车道 token 化 WebUI 根页、Svelte shell 资源加载、壳层 settings launcher 调用链路已脚本化收口；
- macOS 托盘 `Settings` 动作 -> host 回调 -> settings launcher 调用链路已通过 tray smoke 脚本化收口。

## 手工验收建议
1. 启动 core 车道并点击托盘 `Settings`：
- 预期：浏览器打开 token 化 URL（不再依赖固定 `scaffold` token）。
2. 托盘图标显示：
- 预期：`MFX` 不再出现竖排堆叠。
3. 设置页加载：
- 预期：读取 `WebUI` 的 Svelte 产物（自动化分区可见且可交互）。
