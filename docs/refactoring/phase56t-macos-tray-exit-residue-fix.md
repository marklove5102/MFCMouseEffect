# Phase 56t - macOS Tray Exit Residue Fix

## 判定
- 结论：`Bug/回归`。
- 现象：点击菜单栏 `MFX -> Exit` 后，状态栏图标残留且进程未完全退出，需要手动强杀。

## Root Cause
- `MacosTrayService::RequestExit()` 在 macOS 分支是空实现。
- 退出流程仅依赖上层 event-loop 退出路径，若 shutdown drain 出现阻塞，用户可见会出现“图标/进程残留”。

## Fix
- 文件：`/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosTrayService.mm`
- 在 `RequestExit()` 增加主动退出收口：
  - 主线程立即释放 tray menu/status item（先消除可见残留）
  - 调用 `NSApp terminate:nil` 触发应用终止
  - 同步更新 `started/host` 状态，保持 `Stop()` 幂等

## 风险与边界
- 仅影响 macOS tray `Exit` 路径。
- 不改 Windows/Linux 退出行为。
- 不改 core runtime / effect 逻辑。

## Validation
```bash
cmake --build /tmp/mfx-platform-macos-core-automation-build --target mfx_entry_posix_host -j8
```
