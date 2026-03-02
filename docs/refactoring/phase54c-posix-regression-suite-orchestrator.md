# Phase 54c: POSIX Regression Suite Orchestrator

## 判定先行
- 现象：当前回归入口分散在多条命令中（scaffold、linux gate、automation semantic test），执行顺序和参数容易漂移。
- 判定：`Bug或回归风险`（命令分散导致漏跑、误跑概率上升）。

## 目标
1. 提供统一的一键回归入口，覆盖 POSIX 主链路关键门禁。
2. 保持既有子脚本不破坏，避免“大改一个脚本承担全部职责”。
3. 支持按阶段跳过，方便问题定位和增量调试。

## 改动
1. 新增 suite 编排脚本
- 文件：`tools/platform/regression/run-posix-regression-suite.sh`
- 说明：
  - 开始前自动清理残留 `mfx_entry_posix_host` 进程，减少单实例/端口冲突导致的误失败；
  - 默认串行执行：
    - scaffold regression
    - linux compile gate
    - automation platform semantic tests
  - 支持参数：
    - `--platform`
    - `--scaffold-build-dir`
    - `--linux-build-dir`
    - `--jobs`
    - `--skip-scaffold`
    - `--skip-linux-gate`
    - `--skip-automation-test`
    - `--scaffold-skip-smoke`
    - `--scaffold-skip-http`

2. 更新架构文档
- 文件：
  - `docs/architecture/posix-regression-suite-workflow.md`（新增）
  - `docs/architecture/posix-scaffold-regression-workflow.md`（补充 suite 入口）
- 说明：
  - 将 suite 作为默认全量回归入口；
  - scaffold 文档保留单项回归语义，不被 suite 混淆。

## 验证
```bash
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
./tools/platform/regression/run-posix-regression-suite.sh --help
```
- 结果：通过。

## 影响
- 回归入口从“多命令拼接”升级为“单入口编排 + 子脚本分包”，降低人为漏跑风险。
- 便于后续接入更多 Phase 54 契约级检查而不扩大单脚本复杂度。
