# Mouse Companion Procedural Effect Profile Contract (P2)

## 目标
- 解决“模型无限多，但动作逻辑写死在单模型风格里”的架构问题。
- 将动作驱动拆分为三层：
  - 语义层：动作效果参数（与具体模型无关）
  - 骨架适配层：将语义映射到当前模型骨架角色（head/spine/chest...）
  - 求解层：根据输入事件与语义参数实时生成姿态

## 当前实现（2026-03-17）

### 1) 语义层：`ProceduralEffectProfile`
- 代码：
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Pet/PetEffectProfile.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Pet/PetEffectProfile.cpp`
- 资产：
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Assets/Pet3D/source/pet-effects.json`
- 作用：
  - 定义 `idle/follow/click/drag/hover/hold/scroll` 的语义参数（强度、呼吸、脉冲、躯干偏移、头部跟随增益等）
  - 与模型骨骼名称无关

### 2) 骨架适配层：自动骨骼角色识别
- 代码：`PetActionSynthesizer` 内 `BindSkeleton(...)`
- 当前角色槽位：
  - `hips/spine/chest/neck/head`
- 适配策略：
  - token + alias 匹配（不同模型骨骼命名可复用）

### 3) 求解层：统一程序化姿态求解
- 代码：`PetActionSynthesizer::EmitRigPose(...)`
- 特性：
  - 不再依赖单模型硬编码常量
  - 每帧从 `ProceduralEffectProfile` 读取当前动作语义参数并解算
  - 若动作有 Clip 则优先 Clip；无 Clip 自动回落程序化求解

## JSON 契约（`pet-effects.json`）

顶层：
```json
{
  "version": 1,
  "actions": []
}
```

`actions[]` 元素：
- `action`：`idle|follow|click_react|drag|hover_react|hold_react|scroll_react`
- 可选参数（部分）：
  - `action_intensity`
  - `clip_blend_weight`（clip 存在时，最终姿态 = `lerp(procedural, clip, clip_blend_weight)`）
  - `breathe_hz`, `breathe_amplitude`
  - `pulse_decay_hz`, `pulse_gain`
  - `hips_y_offset`, `hips_pitch`, `hips_yaw`, `hips_scale_x/y/z`
  - `spine_pitch/yaw`, `chest_pitch/yaw`, `neck_pitch/yaw`
  - `head_cursor_pitch_gain`, `head_pitch_bias`, `head_yaw_gain`, `head_yaw_bias`

说明：
- 未配置字段使用内置默认值。
- 未配置某个 `action` 时使用该动作的默认语义模板。
- `clip_blend_weight=1.0` 表示完全使用 clip；`0.0` 表示完全使用程序化骨架效果。

## 运行时加载规则
- 默认路径（顺序）：
  1. `Assets/Pet3D/source/pet-effects.json`
  2. `MFCMouseEffect/Assets/Pet3D/source/pet-effects.json`
- 加载行为：
  - 文件存在并解析成功：`effect_profile_loaded=true`
  - 文件缺失/解析失败：回落内置默认 profile，且状态里有错误码可观测

## 可观测性
- `/api/state.mouse_companion_runtime` 新增：
  - `effect_profile_loaded`
  - `configured_effect_profile_path`
  - `loaded_effect_profile_path`
  - `effect_profile_load_error`

## 回归与门禁
- 一键 proof：
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/manual/run-macos-mouse-companion-proof.sh`
- 当前门禁新增：
  - `runtime.effect_profile_loaded == true`
