# Phase 53d: macOS Shortcut Capture Keycode Normalization

## 判定先行
- 现象：在 macOS 设置页录制快捷键时，按 `Cmd+V` 被记录成 `Cmd+Tab`。
- 判定：`Bug或回归`。
- 最短依据：
  - `MacosGlobalInputHook` 直接上送 `CGKeyboardEventKeycode`（mac 硬件键码）到 `KeyEvent.vkCode`；
  - core 录制链路按统一 `VirtualKeyCodes` 解释 `vkCode`；
  - `kVK_ANSI_V == 9`，与统一键码里的 `vk::kTab == 0x09` 冲突，导致误判。

## 目标
1. 录制链路在 macOS 上不再直接使用硬件键码，统一收敛到跨平台 `vk` 语义。
2. 保持 Windows/Linux 既有行为不变，不引入跨平台回归。

## 改动
1. 新增 mac 键码转换器（单一职责）
- 文件：`MFCMouseEffect/Platform/macos/System/MacosVirtualKeyMapper.h`
- 文件：`MFCMouseEffect/Platform/macos/System/MacosVirtualKeyMapper.mm`
- 说明：
  - 提供 `VirtualKeyFromMacKeyCode(uint16_t)`；
  - 将常用字母/数字/小键盘/F1-F20/方向键/功能键/修饰键映射到统一 `VirtualKeyCodes`；
  - 未映射键返回 `0`（避免硬件码误撞到错误 `vk`）。

2. mac 输入钩子接入统一映射
- 文件：`MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.mm`
- 说明：
  - `kCGEventKeyDown` 分支改为调用 `macos_keymap::VirtualKeyFromMacKeyCode`；
  - 不再把 `CGKeyboardEventKeycode` 直接写入 `KeyEvent.vkCode`；
  - 同时补齐键盘事件坐标 `KeyEvent.pt`。

3. 构建接线
- 文件：`MFCMouseEffect/Platform/macos/CMakeLists.txt`
- 说明：将 `MacosVirtualKeyMapper.mm` 纳入 `mfx_shell_macos`。

## 关于 `process:*.exe`（mac 语义）
- 当前 `process:code.exe / process:code / process:code.app` 在 mac 上是设计内兼容行为，不是本次回归根因。
- 依据：`MFCMouseEffect/MouseFx/Core/Automation/AppScopeUtils.h` 通过别名归一化做跨后缀匹配。

## 验证
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- 结果：全部通过。

## 脚本收口（2026-02-24）
1. 新增测试态 API（默认关闭）：
- `POST /api/automation/test-shortcut-from-mac-keycode`
- 环境变量门禁：`MFX_ENABLE_AUTOMATION_SHORTCUT_TEST_API=1`
2. 回归脚本新增断言（`tools/platform/regression/lib/core_http.sh`）：
- `{"mac_key_code":9,"cmd":true}` 必须返回 `shortcut="Win+V"`、`vk_code=86`
- `{"mac_key_code":48,"cmd":true}` 必须返回 `shortcut="Win+Tab"`、`vk_code=9`
3. 结论：
- `Cmd+V` 与 `Cmd+Tab` 在 contract 层已可自动区分，53d 从“手工待验收”收口为“脚本验收”。
