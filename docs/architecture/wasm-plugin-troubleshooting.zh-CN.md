# WASM 插件排错手册

适用范围（v2 路线）：
- 运行时桥接：`mfx_wasm_runtime.dll`
- 诊断入口：`/api/state` -> `wasm`

## 1. `load-manifest` 失败

现象：
- `POST /api/wasm/load-manifest` 返回 `ok=false`
- `wasm.last_error` 提示 manifest/module 路径异常

检查：
- `manifest_path` 是否绝对路径且有效
- `plugin.json` 字段是否完整
- `entry` 是否指向存在的 `effect.wasm`

## 2. 运行时桥接失败

现象：
- `wasm.plugin_loaded=false`
- `wasm.runtime_backend="null"`
- `wasm.runtime_fallback_reason` 非空

检查：
- 先构建仓库解法（`MFCMouseEffect.slnx`）
- 确认 `mfx_wasm_runtime.dll` 在可执行同目录或搜索路径
- 确认桥接导出：
  - `mfx_wasm_runtime_create`
  - `mfx_wasm_runtime_call_on_input`
  - `mfx_wasm_runtime_call_on_frame`
  - `mfx_wasm_runtime_last_error`

## 3. 已加载但无可见效果

先看字段：
- `wasm.last_output_bytes`
- `wasm.last_command_count`
- `wasm.last_parse_error`
- `wasm.last_rendered_by_wasm`
- `wasm.last_executed_text_commands`
- `wasm.last_executed_image_commands`
- `wasm.last_render_error`
- `wasm.last_budget_reason`

若解析成功仍无效果：
- 检查前台/VM 抑制策略
- 检查预算截断标记
- 检查 `plugin.json` 的 `input_kinds` 是否包含当前通道（例如 `scroll`）
- 依赖持续喷射的特效需检查 `enable_frame_tick` 是否开启

## 4. `spawn_image` 回退内置图形

检查：
- `plugin.json` 是否声明 `image_assets`
- 路径是否相对 `plugin.json`
- 格式是否支持（`png/jpg/jpeg/bmp/gif/tif/tiff`）
- `imageId` 是否映射到预期下标

说明：资源不可解析时会主动回退内置渲染器。

## 5. 预算拒绝/截断

标记：
- `wasm.last_call_rejected_by_budget`
- `wasm.last_output_truncated_by_budget`
- `wasm.last_command_truncated_by_budget`

处理：
- 降低单事件命令数
- 降低输出字节数
- 简化单事件计算逻辑

## 6. 解析错误

常见 `wasm.last_parse_error`：
- `truncated_header`
- `invalid_command_size`
- `truncated_command`
- `unsupported_command_kind`
- `command_limit_exceeded`

处理：严格对齐 `WasmPluginAbi.h` 布局，并保证 `sizeBytes` 精确一致。

## 7. 插件目录为空（`No plugins discovered`）

检查：
- 查看 `/api/wasm/catalog` 的 `search_roots`
- 检查设置页 `Catalog root path`
- 确保扫描目录下存在合法 `plugin.json`
- Debug 模式会自动扫描 `examples/wasm-plugin-template/dist`

## 8. 最小自检

1. 构建 `examples/wasm-plugin-template`。
2. 放置 `effect.wasm` + `plugin.json` 到插件目录。
3. 调用 `/api/wasm/load-manifest`、`/api/wasm/enable`。
4. 点击一次并检查 `/api/state` 的 `wasm` 字段。

## 9. 重复 `plugin.json.id`

当前行为：
- catalog 对同一个 `plugin.json.id` 只保留一份
- 扫描根优先级为：
  - 显式配置的 `catalog_root_path`
  - 主插件目录
  - 可执行文件旁路目录
  - Debug 模板 `dist`
- catalog 会忽略不支持的 `api_version`
- catalog 会忽略另一个插件包内部的嵌套 `plugin.json`

检查：
- 如果实际生效的不是你想要的那一份，先查看 `/api/wasm/catalog` 的 `search_roots`
- 需要覆盖内置/Debug 副本时，把目标插件放到配置的 `catalog_root_path`
- 如果磁盘里仍保留旧的 `api_version=1` 插件目录，建议清理
- 模板默认 id 应为 `demo.template.default.v2`，样例 id 形如 `demo.*.<sample>.v2`
