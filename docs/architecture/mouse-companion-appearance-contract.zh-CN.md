# Mouse Companion Appearance Contract (P2)

## 目标
- 支持“修改模型外观”能力：材质纹理覆写 + 配件节点显隐。
- 保持运行时高性能：模型加载后建立索引，运行期按结构化参数应用。

## 配置文件
- 默认路径：
  - `Assets/Pet3D/source/pet-appearance.json`
  - `MFCMouseEffect/Assets/Pet3D/source/pet-appearance.json`

## JSON 契约
顶层支持两种模式：

1. 直接默认配置：
```json
{
  "default": {
    "skinVariantId": "default",
    "enabledAccessoryIds": [],
    "textureOverridePaths": []
  }
}
```

2. 预设模式：
```json
{
  "activePreset": "casual",
  "presets": [
    { "id": "casual", "skinVariantId": "casual", "enabledAccessoryIds": [], "textureOverridePaths": [] }
  ]
}
```

字段说明：
- `skinVariantId`：皮肤变体标识（当前保留并透传到渲染桥，便于后续规则扩展）。
- `enabledAccessoryIds`：配件节点 ID 列表（按节点名匹配）。
- `textureOverridePaths`：纹理覆写列表，格式：
  - `MaterialName=/abs/path/to/texture.png`
  - 或 `MaterialName:/abs/path/to/texture.png`

## 运行时流程
1. `LoadPetAppearanceProfileFromJsonFile(...)` 读取并解析配置。
2. `PetCompanionRuntime::ApplyAppearance(...)` 保存当前外观覆写并下发到模型运行时。
3. macOS 桥接：
   - `mfx_macos_mouse_companion_apply_appearance_v1(...)`
   - 配件：按 `enabledAccessoryIds` 控制节点显隐。
   - 材质：按 `textureOverridePaths` 应用纹理；未覆写项回退到模型原始 diffuse。

## 关键性能策略
- 材质索引与骨架索引均在模型加载后构建缓存，不在每帧全量查找。
- 外观应用为“事件触发”（配置变更/模型重载时），不是每帧刷新。

## 回归检查
- 编译：
  - `cmake --build build-macos --target mfx_entry_runtime_common -j 6`
- 行为：
  1. 缺失 `pet-appearance.json` 时安全降级（不影响动作与渲染）。
  2. 配置存在时可成功应用（无崩溃、无线程告警）。
  3. 材质覆写路径无效时回退默认材质，不导致异常。

