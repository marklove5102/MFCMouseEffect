# Phase 52g: macOS Input Indicator Label Lifetime Fix

## 判定先行
- 现象 A: 运行中关闭 `Accessibility` 后，鼠标/键盘效果消失。  
  判定: `设计行为`
  - 最短依据: macOS 全局输入采集依赖权限；权限撤销后系统进入 degraded，输入特效链路不可用。
  - 备注: 当前产品策略仍要求“恢复权限后重启进程”再恢复采集。
- 现象 B: 点击时光标右上角出现空白指示块（无 L/R/M 标识）。  
  判定: `Bug或回归`
  - 最短依据: `MacosInputIndicatorOverlay::ShowAt` 异步 block 使用了 `const std::string& label` 引用，存在生命周期悬空风险，导致标签文本丢失/异常。

## 代码变更
- 文件: `MFCMouseEffect/Platform/macos/Overlay/MacosInputIndicatorOverlay.mm`
  - 在 `ShowAt(...)` 中新增 `const std::string labelCopy = label;`
  - 异步 UI 更新改为使用 `labelCopy`，避免引用悬空。

## 影响范围
- 仅影响 macOS 输入指示器文本展示路径。
- 不改变 Windows/Linux 行为，不改变权限降级语义。

## 验证
1. 构建 + core smoke
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

## 手工复测（最小）
1. 启动:
```bash
/tmp/mfx-platform-macos-core-build/mfx_entry_posix_host --mode=background
```
2. 权限全开时点击，确认指示器显示非空标签（例如 `L` / `R` / `M`），不再是空白块。
3. 运行中关闭 `Accessibility`，确认进入降级（无输入特效）且进程继续存活。
