# macOS Text Click Fallback Diagnostics (Floating Text No-Show)

## 判定
- 类型：Bug/Regression
- 现象：点击“漂浮文字”无任何可见效果（其他 click 类型仍可触发）。
- 目标：确认 `TextEffect` 是否被触发、fallback 是否创建面板、失败点是什么。

## 改动摘要
- 新增轻量运行时诊断：`TextEffectRuntimeDiagnostics`（点击触发、fallback 调用/面板创建/错误计数、最近坐标与文本预览）。
- macOS fallback 改为在异步主线程前预先构造 `NSString`（避免潜在异步生命周期问题），并记录失败原因。
- macOS `CATextLayer` 字体设置改为使用字体名字符串（`CFStringRef`），避免非法 `CFTypeRef` 导致文字不渲染。
- `/api/state.effects_runtime.text_effect` 现可直接读取上述诊断字段。

## 诊断字段（/api/state）
路径：`effects_runtime.text_effect`
- `click_count`：TextEffect OnClick 调用次数
- `fallback_show_count`：fallback ShowText 调用次数
- `fallback_panel_created`：NSPanel 创建成功次数
- `fallback_error_count`：失败次数（见 `last_error`）
- `fallback_active_panels`：当前存活面板数
- `last_click_pt / last_fallback_pt`：最后坐标
- `last_text_preview`：最近文本（截断预览）
- `last_error`：最近错误（例如 `empty_text` / `utf8_empty` / `ns_text_nil` / `panel_nil`）

## 验证步骤（最小）
1. 启动 host 并打开设置页。
2. 点击几次（确保 click 类型为“漂浮文字”）。
3. 查询状态：
   - `curl "http://127.0.0.1:<port>/?token=..."` 或
   - `curl "http://127.0.0.1:<port>/api/state" | python -m json.tool`
4. 期望：
   - `click_count` 与 `fallback_show_count` 同步递增。
   - `fallback_panel_created` 递增。
   - `last_error` 为空或不再变化。

## 下一步判定路径
- `click_count` 不增：click 路由未触发或 active.click 不为 `text`。
- `fallback_show_count` 增但 `fallback_panel_created=0`：面板创建失败，重点看 `last_error`。
- 两者均增但仍不可见：渲染/坐标问题，需继续对齐 `ScreenToOverlayPoint` 与坐标系。
