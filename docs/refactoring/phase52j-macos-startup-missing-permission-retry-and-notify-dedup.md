# Phase 52j: macOS Startup Missing-Permission Recovery Retry + Notify Dedup

## 判定先行
- 反馈:
  1. 启动时未授予权限会弹两次降级通知；
  2. 不重启进程，后续开启权限后无恢复反应。
- 判定: `Bug或回归`

## 最短依据
1. 双通知根因
- `PosixCoreAppShell` 在 `Start()` 之前注册回调，`Start()` 内进入 degraded 会先触发一次通知；
- `Start()` 返回后又执行初始状态检查再通知一次。

2. 启动缺权不恢复根因
- 启动缺权时 `hook_->Start(...)` 失败后 hook 线程结束；
- 后续健康探针只读取 `LastError()`，没有重试 `Start()`，因此权限恢复后不会自动拉起 hook。

## 修复
1. 启动缺权自动恢复
- 文件: `MFCMouseEffect/MouseFx/Core/Control/AppController.h`
- 文件: `MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
- 在 `RefreshInputCaptureRuntimeState()` 中增加“degraded + error!=0”节流重试：
  - 每 `1000ms` 最多重试一次 `hook_->Start(dispatchHost)`；
  - 重试成功后沿用既有恢复分支，恢复输入效果链路。

2. 降级通知去重
- 文件: `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.cpp`
- 将 `SetInputCaptureStatusCallback(...)` 改为在 `Start()` 成功后注册；
- 启动阶段只保留一次初始降级通知，运行时状态切换仍通过回调通知。

3. 轮询频率调优（性能）
- 将权限相关轮询从 `250ms` 调整为 `500ms`：
  - `AppController` 输入采集健康探针 timer；
  - `MacosGlobalInputHook` 权限探针 timer。
- 目标：降低稳定运行时的唤醒频率，同时保持可接受的恢复时延。

## 回归验证
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
printf 'exit\n' | /tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- 结果: passed

## 手工验收（本次问题）
1. 关闭 Accessibility / Input Monitoring，启动 app。
- 预期: 仅 1 次降级通知。
2. 不退出 app，开启权限。
- 预期: 约 1 秒内自动恢复输入效果（无需重启进程）。
