# Phase 52k: macOS Permission + Indicator Contract Automation

## 判定先行
- 现状: `52f/52g/52h/52i/52j` 的代码已落地，但验收长期依赖人工切换系统权限与目测标签。
- 判定: `证据不足待确认`（实现存在，自动化证据不足，回归成本高）。

## 目标
1. 将权限缺失/撤销/恢复路径转为脚本可重复验证。
2. 将 macOS 输入指示器 `L/R/M` 标签链路转为脚本断言，降低人工目测依赖。
3. 保持生产行为不变，所有新增能力必须由测试态显式开关控制。

## 改动
1. macOS 全局输入 Hook 增加测试态权限模拟（文件驱动）
- 文件: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/System/MacosGlobalInputHook.mm`
- 说明:
  - 新增环境变量: `MFX_TEST_INPUT_CAPTURE_PERMISSION_SIM_FILE`
  - 文件内容支持 `trusted=0/1`（也接受 true/false/yes/no/on/off）。
  - 未设置该变量时保持原生 `AXIsProcessTrusted()` 逻辑不变。
  - 设置后:
    - 启动阶段可模拟“无权限启动失败”（`trusted=0`）。
    - 运行阶段可模拟“撤权/恢复”并驱动 `LastError` 转换。

2. macOS 通知服务增加测试态捕获
- 文件: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Shell/MacosUserNotificationService.cpp`
- 说明:
  - 新增环境变量: `MFX_TEST_NOTIFICATION_CAPTURE_FILE`
  - 每次 `ShowWarning(...)` 追加写入一行 `title=... message=...`。
  - 未设置时行为不变。

3. 输入指示器增加测试探针接口与 macOS 实现
- 文件: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Overlay/IInputIndicatorOverlay.h`
- 文件: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Overlay/MacosInputIndicatorOverlay.h`
- 文件: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Overlay/MacosInputIndicatorOverlay.mm`
- 说明:
  - `IInputIndicatorOverlay` 增加测试态可选接口:
    - `ReadDebugState(...)`
    - `RunMouseLabelProbe(...)`
  - macOS overlay 在主线程实际设置文本后记录 `lastAppliedLabel/applyCount`。
  - `RunMouseLabelProbe` 触发 `L/R/M` 三次渲染并返回实测标签序列。

4. WebSettings 测试态 API 扩展
- 文件: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
- 说明:
  - 新增 `POST /api/input-indicator/test-mouse-labels`
  - 开关: `MFX_ENABLE_INPUT_INDICATOR_TEST_API=1`
  - 响应包含:
    - `supported`
    - `matched`
    - `labels`（期望 `["L","R","M"]`）
    - `last_applied_label`
    - `apply_count`

5. core automation contract 回归扩展
- 文件: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`
- 文件: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
- 说明:
  - 启动前写入 `trusted=0`，断言启动即降级（`reason=permission_denied`）。
  - 捕获通知并断言启动缺权限告警去重（1 条）。
  - 启动后写入 `trusted=1`，断言自动恢复（`active=true`）。
  - 运行中执行 revoke/regrant 断言（不重启进程）。
  - 调用 `/api/input-indicator/test-mouse-labels` 断言 `L/R/M` 标签链路。

## 测试态参数（默认值 + 测试值）
- `MFX_TEST_INPUT_CAPTURE_PERMISSION_SIM_FILE`
  - 默认: 未设置（生产真实权限路径）
  - 测试示例:
    - 文件写 `trusted=0` -> 无权限
    - 文件写 `trusted=1` -> 有权限
- `MFX_TEST_NOTIFICATION_CAPTURE_FILE`
  - 默认: 未设置（不捕获）
  - 测试示例: `/tmp/mfx-notify-capture.log`
- `MFX_ENABLE_INPUT_INDICATOR_TEST_API`
  - 默认: `0`（关闭）
  - 回归脚本默认: `1`
- `MFX_CORE_HTTP_INPUT_CAPTURE_TIMEOUT_SECONDS`
  - 默认: `10`
  - 测试示例: `12`（低性能机器可放宽）

## 验证
```bash
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/docs/doc-hygiene-check.sh --strict
```
- 结果: passed。

## 收口结论
- `52f` 运行中撤权降级路径：脚本收口。
- `52g` 指示器 `L/R/M` 标签链路：脚本收口。
- `52h` 启动缺权限降级收敛：脚本收口。
- `52i` 运行中 revoke/regrant 自动恢复：脚本收口。
- `52j` 启动缺权限通知去重 + 授权后恢复：脚本收口。
