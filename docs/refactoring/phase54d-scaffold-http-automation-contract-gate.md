# Phase 54d: Scaffold HTTP Automation Contract Gate

## 判定先行
- 现象：`run-posix-scaffold-regression.sh` 之前只覆盖 schema/state/token/webui 相关 HTTP 行为，未覆盖 automation 关键 API 契约。
- 判定：`Bug或回归风险`（scaffold/core 双车道并行期间，若不锁定 scaffold 的“未支持”合同，后续切换容易出现误判或隐性漂移）。

## 目标
1. 把 automation API 关键契约纳入 scaffold HTTP 回归。
2. 以当前真实行为为准：scaffold 车道对 automation API 明确返回 `404 not found`。
3. 不改动业务 API，仅增强回归门禁覆盖。

## 改动
1. 扩展 scaffold HTTP 默认路由检查
- 文件：`tools/platform/regression/lib/http.sh`
- 说明：
  - 新增 `POST /api/automation/active-process` 校验：
    - 所有平台：`404 + body contains "not found"`（scaffold 明确未支持）
  - 新增 `POST /api/automation/app-catalog` 校验：
    - 所有平台：`404 + body contains "not found"`（scaffold 明确未支持）

2. 文档同步
- 文件：`docs/architecture/posix-scaffold-regression-workflow.md`
- 说明：
  - 在 HTTP 校验项下补充 scaffold automation API “显式未支持”合同。

## 验证
```bash
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto
./tools/platform/regression/run-posix-regression-suite.sh --platform auto
```
- 结果：通过。

## 影响
- 把 scaffold 车道 automation API 的边界行为从“隐式”变成“可回归验证的显式合同”。
- 降低双车道并行阶段的误判风险，后续 core 车道扩展时更容易做增量切换。
