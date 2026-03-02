# Phase 54a: Linux Compile Gate Script

## 判定先行
- 现象：Phase 54 的 Linux 编译门禁依赖多条手工 `cmake` 命令，重复且容易漂移。
- 判定：`Bug或回归风险`（流程不稳定会导致“看似通过、实际漏项”的跨平台回归风险）。

## 目标
1. 把 Linux 编译门禁收敛为一条稳定命令。
2. 保持脚本分层（入口编排与构建逻辑分离），避免“超大脚本”。
3. 不改动 Windows/macOS 运行路径，仅增强跨平台门禁可执行性。

## 改动
1. 新增 Linux 门禁构建模块
- 文件：`tools/platform/regression/lib/linux_gate.sh`
- 说明：
  - 提供 `mfx_run_linux_compile_gate(repo_root, build_dir)`；
  - 固化 Linux 包构建参数：
    - `-DMFX_PACKAGE_PLATFORM=linux`
    - `-DMFX_ENABLE_CROSS_HOST_PACKAGES=ON`
    - `-DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON`
  - 固化门禁目标：
    - `mfx_shell_linux`
    - `mfx_entry_posix`

2. 新增门禁入口脚本
- 文件：`tools/platform/regression/run-posix-linux-compile-gate.sh`
- 说明：
  - 支持参数：
    - `--build-dir`（默认 `/tmp/mfx-platform-linux-build`）
    - `--jobs`（覆盖并行编译数）
  - 保持入口脚本只负责参数解析与调度，具体构建逻辑下沉到 `lib/linux_gate.sh`。

3. 新增架构工作流文档
- 文件：`docs/architecture/posix-linux-compile-gate-workflow.md`
- 说明：
  - 明确一键命令、验证范围、脚本边界与扩展规则。

## 验证
```bash
./tools/platform/regression/run-posix-linux-compile-gate.sh
```
- 结果：通过。

## 影响
- Linux 编译级门禁从“命令片段”升级为“一键流程”，降低阶段回归成本。
- 为后续 CI 或本地统一门禁接线提供稳定入口。

## 补充（2026-02-24）
- 仓库根 `.gitignore` 已加入 `.DS_Store`，避免 macOS Finder 元数据污染工作区与提交记录。
