# WASM 插件 ABI v3 设计稿

## 状态
- 状态：`Proposed（phase1 已有一部分以 v2 兼容形态落地）`
- 日期：`2026-03-08`
- 目标：把当前 WASM 路线从“受控爆发类命令系统”推进到“跨平台、自定义程度更高的特效运行时”

## 背景
当前 v2 已经具备稳定运行时、按帧驱动和一组可用原语：
- `spawn_text`
- `spawn_image`
- `spawn_image_affine`
- `spawn_pulse`
- `spawn_polyline`
- `spawn_glow_batch`
- `spawn_sprite_batch`

这套能力已经足够覆盖文本、图片、脉冲、折线、批量粒子和批量精灵喷发，但它离“接近原生 C++ 的自由效果”仍有几个关键缺口：
- 仅有最小化的跨平台 retained glow emitter 基线，还缺少更通用的 retained effect / emitter 生命周期契约
- 缺少通用 path/vector 原语（曲线、填充、多段 path）
- 缺少统一的 layer / blend / clip / sort 能力
- 缺少 textured quad / atlas / ribbon 这类更接近高级图形组织的公共面
- 缺少宿主拥有、但可由 WASM 触发的有限后处理 pass

## 非目标
v3 不做以下事情：
- 不允许插件直接控制窗口、交换链或平台原生图形上下文
- 不允许插件直接注入任意 GPU shader / pipeline
- 不把宿主安全预算、降级和回退机制下放给插件
- 不追求“一次升级后完全等价原生 C++ 自由度”

核心边界保持不变：
- `WASM 负责计算`
- `宿主负责渲染执行、资源管理和安全控制`

## 设计原则
1. 公共原语优先
- 不继续堆平台/样式专用命令，优先补能复用的跨平台原语。

2. 先 retained，再继续瞬时命令
- 当前 v2 更偏一次性 burst。v3 第一优先级是“持续存在且可更新”的实例/发射器。

3. 跨平台正式能力必须 Win/macOS 同时成立
- 某能力若只有单平台可执行，只能作为实验能力，不能进入 v3 正式公共面。

4. 安全模型继续收口在宿主
- 预算、节流、降级、诊断、资源解析继续由宿主控制。

5. 逐层扩展，不做一步到位大一统
- 先补最缺的 retained + path + layer/blend，再考虑 mesh 和后处理。

## v3 目标分层

### P1 必做
- retained particle / sprite emitter
- path/vector primitive
- shared layer / blend / clip / sort contract

### P2 建议
- textured quad batch / atlas / uv animation
- ribbon / trail strip primitive
- richer gradient / color-over-life helpers

### P3 谨慎推进
- host-owned post-process passes
- feedback / afterimage / blur / bloom 这类受控后处理

## v3 P1 详细方案

### 1. Retained Emitter（第一优先级）
建议新增跨平台正式命令：
- `upsert_particle_emitter`
- `remove_particle_emitter`

目的：
- 支撑长按光环、悬停环、持续喷射、尾焰、跟随粒子场、拖尾火花等持续效果
- 减少 WASM 为维持连续效果而每帧回灌大量瞬时 batch 命令的压力

建议字段：
- `emitter_id`
- `anchor_mode`
  - `absolute_screen`
  - `follow_cursor`
  - `follow_input_point`
- `particle_kind`
  - `glow`
  - `sprite`
- `image_id`
- `emission_rate_per_sec`
- `burst_count`
- `max_particles`
- `direction_rad`
- `spread_rad`
- `speed_min` / `speed_max`
- `accel_x` / `accel_y`
- `drag`
- `size_min_px` / `size_max_px`
- `rotation_min_rad` / `rotation_max_rad`
- `alpha_min` / `alpha_max`
- `color_start_argb` / `color_end_argb`
- `particle_life_ms`
- `emitter_ttl_ms`
- `blend_mode`
- `sort_key`
- `group_id`

说明：
- 当前 v2 `upsert_glow_emitter/remove_glow_emitter` 已经落地为 Win/macOS 对齐的 retained glow emitter 基线能力。
- v3 仍要把它继续抽象成更通用的 retained particle / sprite emitter 契约，而不是长期停留在 glow emitter 特化形态。

### 2. Path / Vector Primitive
建议新增跨平台正式命令：
- `spawn_path_stroke`
- `spawn_path_fill`

目的：
- 让 WASM 直接组织闪电、丝带、封闭轮廓、平滑轮廓、徽记、多边形填充等效果
- 解决当前 `spawn_polyline` 只能表达折线描边、曲线和填充能力不足的问题

建议数据模型：
- header
- `node_count`
- `nodes[]`

节点操作码：
- `move_to`
- `line_to`
- `quad_to`
- `cubic_to`
- `close`

建议公共样式字段：
- `stroke_width_px`
- `stroke_argb`
- `fill_argb`
- `glow_argb`
- `alpha`
- `line_join`
- `line_cap`
- `fill_rule`
- `delay_ms`
- `life_ms`
- `blend_mode`
- `sort_key`
- `group_id`

### 3. Shared Layer / Blend / Clip Contract
v3 不建议继续只靠局部 `screen_blend` 标志扩展。

建议抽出所有新命令共用的渲染语义字段：
- `blend_mode`
  - `normal`
  - `add`
  - `screen`
- `sort_key`
- `group_id`
- `clip_rect`
- `anchor_mode`

目的：
- 让多类命令之间具备稳定的叠放关系
- 为将来 group 级后处理和受控 layer pass 预留空间

当前桥接状态：
- 最小版 `blend_mode/sort_key/group_id` 尾部已经落在部分 v2 命令上：`spawn_glow_batch`、`spawn_sprite_batch`、`upsert_glow_emitter`
- 当前宿主解释刻意保持最小：`screen|add` 都映射为 screen-like 合成，`sort_key` 只负责 Windows/macOS 叠放顺序，`group_id` 现在已经作为 `remove_group` 的 retained 生命周期键落地，更宽的 group-owned pass 仍留给后续阶段

## v3 P2 方案

### 1. Textured Quad / Atlas Primitive
建议新增：
- `spawn_quad_batch`

目标：
- 支撑 atlas 帧动画、四边形扭曲、卡片翻转、片元化彩屑、贴图带状效果

建议字段：
- `quad_count`
- `image_id` / `atlas_frame`
- `position`
- `size`
- `uv_rect`
- `rotation`
- `alpha`
- `tint`
- `delay_ms`
- `life_ms`
- `blend_mode`
- `sort_key`

### 2. Ribbon / Trail Strip Primitive
建议新增：
- `spawn_ribbon_strip`
- 或 `upsert_ribbon_trail`

目标：
- 用统一公共面表达丝带拖尾、流光线、能量束、挥舞痕迹

## v3 P3 方案

### Host-Owned Post Process
若后续确实需要 bloom、blur、afterimage、feedback 等能力，建议只开放宿主拥有的有限 pass：
- `apply_group_blur`
- `apply_group_bloom`
- `apply_group_afterimage`

约束：
- 插件只能声明“希望对某 group 使用哪种受控 pass”
- 插件不能上传任意 shader
- 插件不能直接请求底层 GPU 资源

## 兼容与迁移策略

### 版本策略
- 保持 v2 继续稳定可用
- v3 使用新的 `api_version=3`
- 不做 “v2 插件自动推断为 v3” 这类隐式兼容

### 宿主落地顺序
1. 宿主先增加 v3 parser、诊断和 capability 暴露
2. 先把 retained emitter 做成 Win/macOS 正式公共能力
3. 再补 path/vector
4. 再补 layer/blend/clip
5. 最后才推进 quad/ribbon/post-process

### 模板迁移顺序
1. 保留 `examples/wasm-plugin-template` 的 v2 入口
2. 新增 v3 分支入口和样例矩阵
3. 等 Win/macOS 两端都通过回归后，再切默认模板到 v3

## 回归与验收要求
每个新公共原语都必须具备：
- Windows + macOS 双实现
- catalog/schema/state capability 暴露
- 正向样例
- 负向 fixture
- 预算截断/超时/降级验证
- 文档样例和排障路径

## 第一阶段建议实现切片
建议先做 `v3-phase1-emitter`：
1. 已完成：将现有 `upsert_glow_emitter/remove_glow_emitter` 提升为跨平台 retained glow emitter 基线能力
2. 已完成：通过可选 render-semantics tail，在部分 v2 命令上补 `blend_mode/sort_key/group_id` 的最小公共字段
3. 下一步：增加 2 个官方样例：
- `hold-aura-field`
- `hover-sprite-fountain`
4. 下一步：增加回归 gate：
- emitter 生命周期
- emitter 锚点跟随
- emitter 预算上限

原因：
- 收益最高
- 对现有 v2 侵入最小
- 能立即把“瞬时 burst”为主的 wasm 路线推进到“持续效果”为主

## 当前决策
- 当前正式公共面仍以 v2 为准
- 本文作为后续实现与 review 的唯一高价值设计入口
- 最小化 retained glow emitter 基线已在 Win/macOS 双端落地，但更通用的 retained emitter 契约仍属于后续 v3 工作
