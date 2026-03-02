# Phase 54b: Automation Platform Regression Tests

## 判定先行
- 现象：自动化 `app_scope` 的平台语义（`.app/.exe`）此前主要依赖手工验收，缺少稳定的脚本化回归入口。
- 判定：`Bug或回归风险`（平台语义回退时难以及时发现）。

## 目标
1. 为 WebUI 自动化平台语义提供轻量、可本地执行的回归测试。
2. 不引入重量级测试框架，保持仓库依赖和脚本复杂度可控。
3. 覆盖关键合同：平台识别、catalog 归一化、scope 读写序列化。

## 改动
1. 新增自动化平台语义测试脚本
- 文件：`MFCMouseEffect/WebUIWorkspace/scripts/test-automation-platform.mjs`
- 说明：
  - 覆盖 `platform` 别名归一化（`darwin/mac/win32/linux`）；
  - 覆盖默认后缀策略（`app/exe/none`）；
  - 覆盖 catalog 归一化；
  - 覆盖 `parseAppScopes` / `serializeAppScopes`；
  - 覆盖 `normalizeAutomationPayload`（schema 平台别名）；
  - 覆盖 `readMappings` 输出 `app_scope/app_scopes` 的平台语义。

2. 增加脚本入口
- 文件：`MFCMouseEffect/WebUIWorkspace/package.json`
- 说明：
  - 新增 `test:automation-platform`：
    - `node scripts/test-automation-platform.mjs`

## 验证
```bash
pnpm --dir MFCMouseEffect/WebUIWorkspace run test:automation-platform
pnpm --dir MFCMouseEffect/WebUIWorkspace run build:automation
```
- 结果：通过。

## 影响
- 将平台语义回归从“纯人工检查”升级为“脚本 + 人工”双保险。
- 为后续 Phase 54 的 Linux 契约级跟随提供更稳定的 WebUI 侧回归支撑。
