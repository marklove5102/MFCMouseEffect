# Phase 54f: Core Automation HTTP Contract Regression

## 判定先行
- 现象：Phase 54d 只锁定了 scaffold 车道的 automation 未支持合同（`404`）；core 车道的 automation API 合同仍主要依赖手测。
- 判定：`Bug或回归风险`（双车道并行下，core automation 可能在不触发 scaffold/suite 现有断言时退化）。

## 目标
1. 给 core 车道增加可自动化的 HTTP 合同回归（`200` 语义）。
2. 仅在测试态暴露 WebSettings URL/token，可发现但不影响默认用户行为。
3. 保持脚本分层：能力模块在 `lib/`，suite 只做编排。

## 改动
1. core 测试态 WebSettings 探针（代码）
- 文件：`MFCMouseEffect/Platform/posix/Shell/PosixCoreWebSettingsProbe.h`
- 文件：`MFCMouseEffect/Platform/posix/Shell/PosixCoreWebSettingsProbe.cpp`
- 文件：`MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.h`
- 文件：`MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.cpp`
- 文件：`MFCMouseEffect/Platform/CMakeLists.txt`
- 说明：
  - 新增 `MFX_CORE_WEB_SETTINGS_PROBE_FILE` 测试态探针输出（`url/token/port`）。
  - 仅当该环境变量显式配置时，core shell 在后台模式主动启动 WebSettingsServer 并写探针文件。
  - 默认未配置时不改变原有路径。

2. core automation HTTP 回归模块（脚本）
- 文件：`tools/platform/regression/lib/core_http.sh`
- 文件：`tools/platform/regression/run-posix-core-automation-contract-regression.sh`
- 说明：
  - 启动 core 车道进程并等待探针文件就绪。
  - 使用 `x-mfcmouseeffect-token` 头校验 token gate。
  - 合同断言：
    - `GET /api/state`：有 token 为 `200`，无 token 为 `401`
    - `POST /api/automation/active-process`：`200 + ok=true`
    - `POST /api/automation/app-catalog`：`200 + ok=true`
  - macOS 增加强断言：catalog 非空，且至少含一个 `.app` 进程名。

3. suite 接线
- 文件：`tools/platform/regression/run-posix-regression-suite.sh`
- 说明：
  - 新增 core automation phase（默认启用）。
  - 新增参数：
    - `--core-automation-build-dir`
    - `--skip-core-automation`
  - 该 phase 当前为 macOS 路径；其他平台显式 skip。

4. 文档同步
- 文件：`docs/architecture/posix-core-automation-contract-workflow.md`
- 文件：`docs/architecture/posix-regression-suite-workflow.md`

## 验证
```bash
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
```
- 结果：通过。

## 影响
- 回归门禁从“scaffold-only”扩展到“scaffold + core 各自合同”，双车道边界更清晰。
- core automation 关键 API 从手测为主升级为脚本化可回归。
