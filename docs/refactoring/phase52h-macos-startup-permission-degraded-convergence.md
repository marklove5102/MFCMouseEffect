# Phase 52h: macOS Startup Permission-Degraded Convergence

## 判定先行
- 现象: 在应用启动时即缺失 `Accessibility` 权限，仍出现异常行为。  
- 判定: `Bug或回归`
- 最短依据:
  - 启动阶段 hook 失败时，仅写入 `input_capture` 状态，但未统一走运行时的降级收敛路径。
  - 导致“启动缺权”和“运行中撤权”两条路径行为不一致。

## 变更目标
- 启动即缺权时，行为与运行中撤权完全一致:
  - 进程保活（degraded）；
  - 输入效果链路立即收敛；
  - 不保留残余定时器行为。

## 代码变更
- 文件: `MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
  1. 在 `hook_->Start(...)` 失败且非 Windows 分支，调用 `EnterInputCaptureDegradedMode(hookError)`。
  2. 在 `EnterInputCaptureDegradedMode(...)` 中补充停止 `kHoverTimerId`（此前仅停 `kHoldTimerId`）。

## 影响面
- macOS/Linux core lane 的启动缺权路径收敛更一致。
- Windows 行为不变（仍按原语义启动失败即退出）。

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
1. 关闭 `Accessibility`（`Input Monitoring` 可按现状）。
2. 启动:
```bash
/tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background
```
3. 预期:
   - 进程存活；
   - 进入降级（无输入特效）；
   - 无额外异常残留表现。
