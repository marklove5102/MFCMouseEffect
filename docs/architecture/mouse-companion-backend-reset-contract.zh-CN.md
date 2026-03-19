# Mouse Companion 后端重置契约（2026-03-18）

## 背景与目标
- 目标：按产品方向将 `Mouse Companion` 后端实现整体清空，准备从零重建。
- 范围：仅后端代码；前端（`WebUIWorkspace` / `WebUI`）保留。

## 本次重置范围
- 已从仓库删除：
  - `MFCMouseEffect/MouseFx/Core/Pet/*`
  - `MFCMouseEffect/Platform/macos/Pet/*`
- 已从构建入口移除：
  - `MFCMouseEffect/Platform/CMakeLists.txt` 中全部 `Core/Pet` 源文件
  - `MFCMouseEffect/Platform/macos/CMakeLists.txt` 中 `MacosMouseCompanionBridge.swift`
  - `MFCMouseEffect/MFCMouseEffect.vcxproj` 与 `.filters` 中全部 `Core\\Pet` 条目

## 兼容保留（为了前端与配置契约不崩）
- 保留 `mouse_companion` 配置结构与读写路径（schema/state/apply 仍可通）。
- `AppController` 的宠物分发入口仍存在，但全部为 no-op。
- `mouse_companion_runtime` 诊断字段仍返回，统一标记：
  - `runtime_present=false`
  - 主要错误字段为 `backend_removed_pending_rewrite`
- 测试路由 `/api/mouse-companion/test-dispatch` 仍可访问，但不再驱动旧 pet 运行时。

## 验证结论
- 本地构建通过：`cmake --build build-macos -j8`
- 结论：旧 pet 后端已退出构建与运行时路径，工程可继续进入“从零实现”阶段。

## 重建约束（下一阶段）
- 先定义最小后端契约（动作机、骨架绑定、渲染桥接）再落代码。
- 保持 `mouse_companion` 前端字段兼容，避免前端联调中断。
- 新实现优先小步提交，逐步恢复：`click -> hold -> scroll -> idle/hover/follow`。

## 进展补充（2026-03-18，Phase0）
- 已新增插件化占位宿主：`MouseCompanionPluginHostPhase0`（仅提供诊断与事件轨迹，不恢复旧骨架渲染逻辑）。
- `/api/state.mouse_companion_runtime` 与 `/api/mouse-companion/test-dispatch` 现在可返回插件化占位字段：
  - `plugin_host_ready`
  - `active_plugin_id`
  - `compatibility_status`
  - `plugin_event_count`

## 进展补充（2026-03-18，Phase1）
- 已新增 macOS 可视化占位宿主：`Platform/macos/Pet/MacosMouseCompanionPhase1Bridge.swift`（非骨架渲染，先恢复可见动作反馈）。
- 当前可见行为：
  - `position_mode=fixed_bottom_left` 可固定显示在左下角；
  - `click/drag/hold/scroll/follow/idle` 动作可驱动占位形象反馈。
- 当前边界更新：
  - 视觉宿主已支持“模型优先 + 占位兜底”，可加载 `Assets/Pet3D/source/pet-main.*` 到 SceneKit；失败时回退占位形象；
  - 旧后端动作库/effect/appearance 运行时仍未恢复，仍保持“旧骨架后端已移除”的重建边界。
