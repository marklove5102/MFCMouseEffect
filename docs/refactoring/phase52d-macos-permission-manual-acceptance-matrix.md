# Phase 52d: macOS Permission Manual Acceptance Matrix (M1 Closure)

## Snapshot
- Date: 2026-02-23
- Related commits:
  - `83ed3a9` (permission-aware degraded status + WebUI capability consume)
  - `8abf186` (mac core lane AppController bootstrap)

## Scope
- Validate macOS global-input permission behavior for M1 closure.
- Confirm degraded path is observable and stable when permissions are missing.
- Confirm no false degraded state when permissions are granted.

## 判定先行
- `设计行为`:
  - 权限缺失时继续运行（不崩溃），并显示降级提示。
  - 权限授予后降级提示消失，输入采集恢复 `active=true`。
- `Bug或回归`:
  - 权限已完整授予但 `input_capture.active` 仍长期为 `false`。
  - 权限缺失时进程崩溃或卡死。
  - 降级状态下仍错误展示为正常（无告警且状态字段错误）。
- `证据不足待确认`:
  - 未重启应用就直接判定权限结果（macOS 权限通常需要重启进程生效）。

## Preconditions
1. Build targets (mac core lane + scaffold guardrail):
```bash
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-macos-core-build \
  -DMFX_PACKAGE_PLATFORM=auto \
  -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON \
  -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON \
  -DMFX_ENABLE_POSIX_CORE_RUNTIME=ON
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_shell_macos mfx_entry_posix_host -j8
```
2. Web settings path可用时，记录 settings URL 中 `token` 值（用于 API 采样）。
3. 本矩阵（静态权限组合）建议“变更权限后重启应用再观察”；运行时撤权/复权热恢复验收改看 `phase52i`（无需重启）。

## Matrix
| Case | Accessibility | Input Monitoring | Expected runtime | Expected shell notify | Expected `/api/state` |
|---|---|---|---|---|---|
| M52D-01 | Off | Off | Degraded | Yes | `input_capture.active=false`, `reason=permission_denied`, `input_capture_notice` present |
| M52D-02 | On | Off | Degraded | Yes | `input_capture.active=false`, `reason=permission_denied`, `input_capture_notice` present |
| M52D-03 | Off | On | Degraded | Yes | `input_capture.active=false`, `reason=permission_denied`, `input_capture_notice` present |
| M52D-04 | On | On | Healthy | No | `input_capture.active=true`, `reason=none`, `input_capture_notice` absent |

## Manual Steps (minimal)
1. 打开 macOS: `系统设置 -> 隐私与安全性 -> 辅助功能/输入监控`。
2. 按矩阵设置权限组合。
3. 重启 MFCMouseEffect。
4. 观察：
   - 是否出现 shell 通知（降级场景应出现）。
   - 设置页状态栏是否优先显示权限降级提示（若设置页可用）。
5. 可选 API 采样（建议执行，便于留证）：
```bash
TOKEN="<settings_url_query_token>"
curl -s -H "X-MFCMouseEffect-Token: ${TOKEN}" http://127.0.0.1:9527/api/state \
  | python3 -c 'import json,sys;d=json.load(sys.stdin);print(json.dumps({"input_capture":d.get("input_capture"),"input_capture_notice":d.get("input_capture_notice")},ensure_ascii=False,indent=2))'
```

## Regression Guards (already executed)
```bash
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- Result: pass (local run on 2026-02-23).

## Acceptance Record Template
- Case ID:
- Permission combo:
- Shell notify observed: Yes/No
- `input_capture.active`:
- `input_capture.reason`:
- `input_capture_notice` present: Yes/No
- 判定: 设计行为 / Bug或回归 / 证据不足待确认
- 备注:

## Execution Record (2026-02-23, user-verified)
- Case ID: `M52D-01`
- Permission combo: Accessibility=Off, Input Monitoring=Off
- Shell notify observed: Yes
- `input_capture.active`: (not sampled by API in this run)
- `input_capture.reason`: (not sampled by API in this run)
- `input_capture_notice` present: (not sampled by API in this run)
- 判定: `设计行为`
- 备注: 关闭权限后弹出降级提示  
  `Global input capture is degraded. Grant Accessibility and Input Monitoring permissions, then restart app.`

- Case ID: `M52D-04`
- Permission combo: Accessibility=On, Input Monitoring=On
- Shell notify observed: No degraded popup (user reported click effect active)
- `input_capture.active`: (not sampled by API in this run)
- `input_capture.reason`: (not sampled by API in this run)
- `input_capture_notice` present: (not sampled by API in this run)
- 判定: `Bug或回归`
- 备注: 点击特效可触发，但存在“位置错位”。该问题独立于权限链路，属于坐标映射问题。

- Case ID: `M52D-04-R1`（re-check）
- Permission combo: Accessibility=On, Input Monitoring=On
- Shell notify observed: No degraded popup
- `input_capture.active`: (not sampled by API in this run)
- `input_capture.reason`: (not sampled by API in this run)
- `input_capture_notice` present: (not sampled by API in this run)
- 判定: `设计行为`
- 备注: 用户复测“都打开没问题”；`phase52e` 坐标错位问题视为关闭。

## New Finding: Click Coordinate Offset (macOS)
- 分类: `Bug或回归`
- 最短依据:
  - 事件坐标直接来自 `CGEventGetLocation` 并原样下发  
    `MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.mm`
  - 点击特效与输入指示器直接用该坐标设置 `NSWindow/NSPanel` frame，未走统一坐标归一化层  
    `MFCMouseEffect/Platform/macos/Effects/MacosClickPulseEffect.mm`  
    `MFCMouseEffect/Platform/macos/Overlay/MacosInputIndicatorOverlay.mm`
- 影响: 权限通过后，功能可用但视觉定位不准确，影响 M1 可用性验收质量。
- 状态更新: 修复方案已在 `phase52e-macos-coordinate-space-unification.md` 落地，等待手工复测确认关闭。

## New Finding: Runtime Permission Revoke Residue (macOS)
- 分类: `Bug或回归`
- 最短依据:
  - 运行中撤销 `Accessibility` 后，进程按设计继续存活，但曾出现遮挡残留且需重启恢复。
  - 原因链路:
    - hook 运行时撤权状态未稳定传递到 core 收敛链路；
    - click pulse `Shutdown` 未强制回收已创建窗口。
- 状态更新: 修复方案已在 `phase52f-macos-runtime-permission-revocation-hardening.md` 落地，等待手工复测确认关闭。

## New Finding: Startup Permission-Missing Path Inconsistency (macOS)
- 分类: `Bug或回归`
- 最短依据:
  - 启动即缺少 `Accessibility` 时，降级路径与运行时撤权路径不完全一致，存在收敛不一致风险。
- 状态更新: 修复方案已在 `phase52h-macos-startup-permission-degraded-convergence.md` 落地，等待手工复测确认关闭。

## New Finding: Runtime Revoke Requires Process Restart (macOS UX Gap)
- 分类: `Bug或回归`
- 最短依据:
  - 运行中撤销后，仅降级但未给出稳定的运行时转移告警与热恢复路径，用户感知为“只能重启进程”。
- 状态更新: 修复方案已在 `phase52i-macos-runtime-permission-hot-recovery-and-notify.md` 落地，等待手工复测确认关闭。

## New Finding: Input Indicator Empty Label (macOS)
- 分类: `Bug或回归`
- 最短依据:
  - 现象为“点击出现指示块但无 L/R/M 标识”。
  - `MacosInputIndicatorOverlay::ShowAt` 的异步 block 使用了 `const std::string& label`，存在生命周期悬空，可能导致文本丢失。
- 状态更新: 修复方案已在 `phase52g-macos-input-indicator-label-lifetime-fix.md` 落地，等待手工复测确认关闭。

## New Finding: Startup Missing-Permission No-Recovery + Duplicate Notify (macOS)
- 分类: `Bug或回归`
- 最短依据:
  - 启动缺权时会出现双通知（启动回调通知 + 启动后状态检查通知重复）。
  - 启动缺权后 hook 未运行，后续仅轮询 `LastError` 但未重试 `Start`，导致授予权限后无自动恢复。
- 状态更新: 修复方案已在 `phase52j-macos-startup-missing-permission-retry-and-notify-dedup.md` 落地，等待手工复测确认关闭。
