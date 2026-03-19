# Mouse Companion 位置模式契约（P2）

## 目标
- 为 `mouse_companion` 增加显式 `position_mode`，避免“骨架动作验证时仍被跟随逻辑扰动”。
- 当前阶段优先保证 click 展示可稳定观测：乌萨奇固定在桌面左下角。

## 判定
- 类型：`Bug/回归`。
- 依据：现有 macOS 视觉宿主每帧按光标重算窗口原点，导致在骨架接入阶段肢体调试受到位移干扰。

## 配置契约（mouse_companion）
- 新增字段：`position_mode`
- 可选值：
  - `fixed_bottom_left`（默认）
  - `follow`
- sanitize 规则：
  - 非法值回退到 `fixed_bottom_left`

## 行为契约

### `fixed_bottom_left`
- 窗口锚定到桌面联合边界的左下角（多屏下使用所有屏幕 `frame` 的 union）。
- `offset_x/offset_y` 作为锚点边距继续生效。
- 点击/长按时允许 `press_lift_px` 在 Y 轴上抬，便于观察 click/hold 动作，但不再随光标做 X 方向漂移。

### `follow`
- 保持原跟随行为：根据光标位置计算窗口原点，再按 `edge_clamp_mode` 执行边界策略。

## 端到端对齐范围
- `EffectConfig` 结构、JSON parse/serialize、sanitize
- `/api/state` schema/state 映射（含 `position_modes` 选项）
- WebSettings `Mouse Companion -> Follow` 面板读写
- macOS Swift C bridge `mfx_macos_mouse_companion_configure_follow_profile_v1`（新增 `positionModeCode`）
- proof 脚本默认载荷显式写入 `position_mode:"fixed_bottom_left"`

## 回归步骤
1. 打开 `Mouse Companion -> Follow`，确认可见 `Position Mode` 下拉，默认 `fixed_bottom_left`。
2. 点击 `Apply` 后移动鼠标，确认乌萨奇保持在左下角（仅点击/长按时出现抬升）。
3. 切换为 `follow` 再 `Apply`，确认恢复跟随行为。
4. 再切回 `fixed_bottom_left`，确认设置持久化后仍固定左下角。
