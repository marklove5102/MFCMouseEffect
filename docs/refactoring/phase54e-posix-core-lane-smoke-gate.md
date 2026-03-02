# Phase 54e: POSIX Core-Lane Smoke Gate

## 判定先行
- 现象：Phase 54 的统一回归只覆盖 scaffold/linux/automation 语义；`MFX_ENABLE_POSIX_CORE_RUNTIME=ON` 仍主要依赖手工构建与手工启动退出验证。
- 判定：`Bug或回归风险`（core 车道可在不触发现有 suite 的前提下发生“可编译但不可运行”退化）。

## 目标
1. 把 core 车道的“启动-存活-退出”加入自动化回归门禁。
2. 维持分包清晰：suite 只编排，core smoke 由独立脚本负责。
3. 保持测试态可调：等待窗口/退出超时支持环境变量快速配置。

## 改动
1. 回归构建模块拆分
- 文件：`tools/platform/regression/lib/build.sh`
- 说明：
  - 拆分 `configure` 与 `build targets` 能力。
  - 新增 `mfx_configure_and_build_entry_host(...)`，供 core smoke 只构建 `mfx_entry_posix_host`。

2. 新增 core lane smoke 执行模块
- 文件：`tools/platform/regression/lib/core_smoke.sh`
- 说明：
  - 使用 FIFO 驱动 `mfx_entry_posix_host -mode=background`。
  - 校验存活窗口（默认 `1s`）与 stdin `exit` 可退出（默认超时 `5s`）。
  - 超时时执行兜底 `TERM`，避免进程残留。

3. 新增独立 phase 脚本
- 文件：`tools/platform/regression/run-posix-core-smoke.sh`
- 说明：
  - 默认开启 `-DMFX_ENABLE_POSIX_CORE_RUNTIME=ON`。
  - 预清理残留 `mfx_entry_posix_host`，降低单实例冲突噪声。
  - 支持 `--platform`、`--build-dir`，支持测试态参数：
    - `MFX_CORE_SMOKE_START_WAIT_SECONDS`
    - `MFX_CORE_SMOKE_ALIVE_SECONDS`
    - `MFX_CORE_SMOKE_EXIT_TIMEOUT_SECONDS`

4. suite 编排接入
- 文件：`tools/platform/regression/run-posix-regression-suite.sh`
- 说明：
  - 新增 core smoke phase（默认启用）。
  - 新增参数：
    - `--core-build-dir`
    - `--skip-core-smoke`

5. 文档同步
- 文件：`docs/architecture/posix-core-lane-smoke-workflow.md`
- 文件：`docs/architecture/posix-regression-suite-workflow.md`

## 验证
```bash
./tools/platform/regression/run-posix-core-smoke.sh --platform auto
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
```
- 结果：通过。

## 影响
- core 车道从“人工 smoke”升级为“脚本化门禁”，减少双车道并行期的运行时盲区。
- suite 仍保持 orchestration-only，不引入实现细节耦合。
