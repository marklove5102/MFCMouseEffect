# Phase 52i: macOS Runtime Permission Hot-Recovery + Notify

## 判定先行
- 反馈: 运行中误关 `Accessibility` 后，用户只能重启进程，体验不可接受。  
- 判定: `Bug或回归`（用户体验级行为缺口）
- 目标:
  1. 运行中撤权后，立刻降级停用输入特效链路；
  2. 立刻给出权限告警；
  3. 权限恢复后尽量自动恢复采集，不强依赖手动重启进程。

## 关键改动
1. `AppController` 加入输入采集健康探针定时器  
   文件: `MFCMouseEffect/MouseFx/Core/Control/AppController.h`  
   文件: `MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
   - 新增 `kInputCaptureHealthTimerId`，用于每 `500ms` 轮询 hook 运行时状态。
   - `RefreshInputCaptureRuntimeState()` 增加恢复分支:
     - 当 `LastError()==0` 且当前处于 degraded（`input_capture.active=false`）时，
       自动恢复 `input_capture.active=true`、重开 hover 定时器、重建 effect 初始化。
   - 保留撤权时统一收敛: 立即停用输入能力与可见效果链路。

2. `MacosGlobalInputHook` 增加权限探测定时器（RunLoop 内）  
   文件: `MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.h`  
   文件: `MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.mm`
   - 新增 `PermissionProbeTimerCallback` + `OnPermissionProbeTimer()`。
   - 每 500ms 检测 `AXIsProcessTrusted()`:
     - 未授权: 置 `permission_denied`，并关闭 event tap；
     - 授权恢复: 自动重新启用 event tap，清理错误码。
   - 让“撤权 -> 授权恢复”在同一进程内具备热恢复基础。

3. `PosixCoreAppShell` 增加运行时状态回调告警  
   文件: `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.h`  
   文件: `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.cpp`
   - 通过 `AppController::SetInputCaptureStatusCallback(...)` 订阅状态变化，不再额外启监视线程。
   - 当状态进入 degraded 时，立即弹权限告警。
   - 权限文案从“需要重启 app”改为“授权后可恢复”，与热恢复策略一致。

## 验证
1. mac core build + smoke
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
printf 'exit\n' | /tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background
```
- 结果: passed

2. scaffold 回归
```bash
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
```
- 结果: passed

3. Linux 编译跟随
```bash
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build \
  -DMFX_PACKAGE_PLATFORM=linux \
  -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON \
  -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- 结果: passed

## 手工复测（最小）
1. 启动:
```bash
/tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background
```
2. 权限全开，确认点击/快捷键映射正常。
3. 运行中关闭 `Accessibility`：
   - 预期: 输入效果立即停用 + 右上角权限告警出现。
4. 不重启进程，重新开启 `Accessibility`：
   - 预期: 输入采集自动恢复（无需手动重启 app 进程）。

> 补充：启动即缺权后的“授权后自动恢复 + 启动通知去重”在 `phase52j-macos-startup-missing-permission-retry-and-notify-dedup.md` 落地。
