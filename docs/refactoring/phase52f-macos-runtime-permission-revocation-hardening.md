# Phase 52f: macOS Runtime Permission Revocation Hardening

## 判定先行
- 结论: `Bug或回归`
- 现象: 应用运行中撤销 `Accessibility` 后，进程仍存活（预期），但出现遮挡/交互异常，需要重启才恢复。
- 最短依据:
  - `MacosGlobalInputHook` 仅在启动阶段做权限判定，运行时撤权没有稳定上报到 core 降级链路。
  - `MacosClickPulseEffect::Shutdown()` 仅改标志位，未强制回收已创建的脉冲窗口。

## 目标
- 保持既有策略: 权限撤销后进程继续存活（degraded），不崩溃。
- 补齐运行时收敛: 撤权后立即停用输入能力并清理可见覆盖层，避免遮挡残留。
- 不引入 Windows 行为回归。

## 代码变更
1. 运行时权限状态上报（mac hook）  
   `MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.mm`
   - 在 `kCGEventTapDisabledByTimeout / kCGEventTapDisabledByUserInput` 分支中增加 `AXIsProcessTrusted()` 判断。
   - 未信任时写入 `kErrorPermissionDenied`；恢复正常事件时回写 `kErrorSuccess`。
2. Core 侧运行时降级收敛  
   `MFCMouseEffect/MouseFx/Core/Control/AppController.h`  
   `MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
   - 新增 `RefreshInputCaptureRuntimeState()` 与 `EnterInputCaptureDegradedMode(...)`。
   - 在 hover/hold 定时器路径轮询 hook 运行时错误。
   - 一旦从 active 转为错误态，立即执行收敛:
     - 关闭 hold 定时器与跟踪状态；
     - 关闭键盘独占采集；
     - 隐藏输入指示器并 reset automation；
     - 对已加载 effect 执行 `Shutdown()`。
3. 点击脉冲窗口生命周期兜底  
   `MFCMouseEffect/Platform/macos/Effects/MacosClickPulseEffect.mm`
   - 增加进程级窗口注册表，记录当前脉冲窗口。
   - `Shutdown()` 时主线程强制回收全部脉冲窗口。
   - 延迟销毁回调改为“先从注册表取出再释放”，避免重复释放或残留。

## 回归验证
1. mac core build
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
printf 'exit\n' | /tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background
```
- 结果: passed

2. scaffold 稳定车道
```bash
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
```
- 结果: passed

3. Linux 编译级跟随
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
2. 在权限全开时验证点击特效正常。
3. 不关闭应用，关闭 `Accessibility`（`Input Monitoring` 可保持开启）。
4. 预期:
   - 右上角出现降级告警；
   - 进程继续存活（设计行为）；
   - 不出现遮挡残留，不需要重启才能恢复桌面交互。
5. 再恢复权限后，按当前策略重启应用再复测（权限链路文案仍为“then restart app”）。
