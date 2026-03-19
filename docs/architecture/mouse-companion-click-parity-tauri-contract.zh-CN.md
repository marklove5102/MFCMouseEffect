# Mouse Companion Click 对齐 Tauri 契约（P2）

## 目标
- 在 `MFCMouseEffect` 的 Phase1 占位宿主上，先把 click 的“触发门槛 + 可见反馈节奏”对齐到 tauri `three-pet-runtime`。
- 当前阶段不恢复旧骨架后端；先保证 `fixed_bottom_left` 下 click 观感可回归。

## 判定
- 类型：`Bug/回归`。
- 依据：此前虽然有 `click_react` 状态切换，但缺少 tauri 风格的可见节奏（耳/手/腿联动与连击头部渐红连续性不足）。

## 对齐基线（与 tauri 一致）
1. 点击门槛：
   - `press <= 220ms`
   - `travel <= 10px`
   - 非 `hold` 激活
   - 非滚动抑制窗口
2. 连击与头部渐红：
   - `clickStreakBreakMs = 650`
   - `headTintPerClick = 0.11`
   - `headTintMax = 0.70`
   - `headTintDecayPerSecond = 0.36`
3. 动作映射：
   - `click -> click_react`
   - `hover -> idle`
   - `follow -> follow`
   - `hold -> hold_react`
   - `scroll -> scroll_react`

## 当前落地（2026-03-19）
- 后端 click 判定与 streak/tint 状态机已接入 `AppController.DispatchState`。
- macOS Phase1 视觉宿主已接入 click 视觉包络：
  - click 触发瞬时强度已调优为 `0.84 + 0.05 * (streak-1)`（上限 `1.0`，test 档 `0.92 + 0.06 * (streak-1)`）；
  - SceneKit 点击主路径使用 tauri 同源 `clickReact` clip 采样（动作库 `pet-actions.json`，keyframe 二分 + quaternion slerp + bone_remap）；
  - `clickReact` one-shot 在动作选择上具备最高优先级（播放窗口内不被 follow/drag 覆盖）；
  - SceneKit 3D 路径增加独立 `60fps` 帧驱动（不再依赖输入事件触发刷新），click one-shot 可完整跑完 `0.3s`；
  - 模型导入后会清除资产自带动画，避免默认动画与 `clickReact` 叠加导致观感偏离；
  - 当 SceneKit 只能加载 `pet-main.usdz` 时，宿主会从同名 `pet-main.glb` 读取 joint 层级与名称，回填 `usdz` 匿名骨骼节点，保证 `Head/Chest` 等动作轨道仍能命中真实骨骼；
  - SceneKit 模型路径已改为“下压/压扁/回弹”主形态（对应 tauri `clickReact` clip 的 chest scale 特征），并移除 click 时的扭头式主姿态；
  - 修复点击污染链路：`button_down` 不再强制发送 `drag`，仅在按压后发生位移时进入 `drag`；
  - 修复低强度拖拽误扭头：SceneKit `drag` 偏航改为随 `actionIntensity` 线性缩放，不再固定大幅偏航；
  - 修复行为残留混入：SceneKit 在 click one-shot 窗口内屏蔽语义骨骼 `apply_pose` 叠加；同时 C++ 侧删除 click 语义 pose 参数与计算链，仅保留 hold/scroll pose 通道；
  - head tint 按后端 `tintAmount` 实时叠加，不再只做单点闪烁。
- 视觉呈现路径升级：
  - 优先加载 `Assets/Pet3D/source/pet-main.*` 到 SceneKit（当前默认命中 `pet-main.usdz`）；
  - 加载失败自动回退占位形象 `phase1://placeholder/usagi`，保证可见反馈不中断。

## 视觉 Motion Profile（prod/test 双档）
- 切换边界：复用 `mouse_companion.use_test_profile`（`false=prod`，`true=test`）。
- prod（默认）：
  - click pulse：`base=0.84`、`streak_step=0.05`、`max=1.0`
  - 语义 pose 衰减：`hold=1.2/s`、`scroll=1.8/s`
  - 语义 pose 通道增益：`hold=1.0`、`scroll=1.0`
- test（快速验收）：
  - click pulse：`base=0.92`、`streak_step=0.06`、`max=1.0`
  - 语义 pose 衰减：`hold=1.6/s`、`scroll=2.4/s`
  - 语义 pose 通道增益：`hold=1.12`、`scroll=1.10`
- 设计目的：
  - prod 保持稳定体感与回归可预期；
  - test 缩短等待时间、放大动作幅度，便于快速判断 click/hold/scroll 全链路。

## 关键实现路径
- click 门槛 + streak/tint：
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.DispatchState.cpp`
- action library 接入 + 运行时状态：
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.Lifecycle.cpp`
- Phase1 占位宿主 + click 视觉联动：
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Pet/MacosMouseCompanionPhase1Bridge.swift`

## 回归步骤
1. 设定 `mouse_companion.enabled=true` 且 `position_mode=fixed_bottom_left`。
2. 快速短按左键，确认动作进入 `click_react`，并观察耳/手/腿同步出现明显点击联动。
3. 长按或大位移释放，确认不进入 `click_react`。
4. 连续点击 4~6 次，确认头部颜色逐步加深；停止点击后颜色缓慢回落。
5. `/api/state.mouse_companion_runtime` 验证：
   - `last_action_name` 可到 `click_react`
   - `click_streak` 与 `click_streak_tint_amount` 按预期变化
   - `visual_host_active=true`
   - `model_loaded=true` 时 `loaded_model_path` 指向 `pet-main.*`；否则回退到占位路径
6. `/api/mouse-companion/test-dispatch` 快速校验双档：
   - prod 下首次 click `last_action_intensity` 约 `0.84`
   - test 下首次 click `last_action_intensity` 约 `0.92`
   - 两档下 scroll 事件都应返回 `last_action_name=scroll_react`

## 非目标（本阶段）
- 旧 skeleton clip 播放恢复。
- 模型/动作库/外观资源完整加载链路恢复。
