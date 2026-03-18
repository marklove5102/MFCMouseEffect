# Mouse Companion 模型导入管线契约（P2）

## 目标
- 把“多格式输入 -> canonical `.glb`”从 importer 主流程中解耦。
- 通过可插拔转换器，支持后续逐步接入真实后端（`vrm/usdz/fbx -> glb`）。

## 契约（C++）
- 文件：`MFCMouseEffect/MouseFx/Core/Pet/PetInterfaces.h`
- 新增：
  - `ModelConversionResult`
  - `IModelFormatConverter`
    - `Supports(ModelFormat)`
    - `ConvertToCanonicalGlb(sourcePath, sourceFormat, outResult)`
  - `CreateDefaultModelFormatConverter()`
  - `CreateModelAssetImporter(std::unique_ptr<IModelFormatConverter>)`

## 默认转换器管线
- 文件：`MFCMouseEffect/MouseFx/Core/Pet/PetModelFormatConverter.cpp`
- 默认实现是组合管线（`CompositeModelFormatConverter`）：
  1. `GlbPassthroughFormatConverter`
  - 处理 `glb`，直通 canonical 路径。
  2. `VrmBinaryCanonicalFormatConverter`
  - 处理 `vrm`（二进制 glTF）：
    - 先校验 GLB header（magic/version/declared length）；
    - 默认导出到 `canonical/<stem>.glb`；
    - 若目标产物未过期则复用缓存；
    - 若目录/复制失败则回退使用源 `.vrm` 路径并附带 warning。
  3. `GltfJsonCanonicalFormatConverter`
  - 处理 `gltf`：
    - 解析 JSON 并校验 `asset` 节点；
    - 收集 `buffers/images` 的相对外部资源 URI；
    - 复制依赖资源到 `canonical/` 对应相对路径；
    - 导出 JSON-only `.glb` 到 `canonical/<stem>.glb`；
    - 若 canonical 产物新于源 `gltf` 与依赖资源，则复用缓存。
  4. `SidecarCanonicalFormatConverter`
  - 处理 `gltf/usdz/vrm/fbx` 的 sidecar 兜底探测。
  - 按顺序探测 sidecar：
    - `<stem>.glb`
    - `canonical/<stem>.glb`
    - `<stem>.canonical.glb`
  5. `ToolBacked` 转换器（`usdz/fbx`）
  - `usdz`：
    - 默认命令（macOS）：`xcrun usdz_converter {src} {dst}`
    - 可由环境变量 `MFX_PET_USDZ_TO_GLB_COMMAND` 覆盖命令模板。
  - `fbx`：
    - 默认命令：`FBX2glTF --binary --input {src} --output {dst}`
    - 可由环境变量 `MFX_PET_FBX_TO_GLB_COMMAND` 覆盖命令模板。
  - 模板占位符：
    - `{src}`：源模型路径（自动 shell quote）
    - `{dst}`：目标 canonical `.glb` 路径（自动 shell quote）
  - 多后端模板输入：
    - 环境变量支持单条或多条命令模板；
    - 多条模板可用换行或 `||` 分隔，按顺序尝试（第一个成功即返回）。
  - 运行策略：
    - 若 `canonical/<stem>.glb` 缓存未过期则复用；
    - 先做命令后端可用性预检（可执行文件 token）；
    - 若命令后端不可用/执行失败则返回失败，并由后续 sidecar 兜底链路接管。

## Importer 行为
- 文件：`MFCMouseEffect/MouseFx/Core/Pet/PetSkeletonModelRuntime.cpp`
- `DefaultModelAssetImporter` 不再内置格式判断细节，只做三件事：
  1. 格式识别与基础支持性检查；
  2. 委托 `IModelFormatConverter` 得到 canonical `.glb`；
  3. 调用 `ValidateCanonicalGlb` 做运行时前置校验。
- 诊断兜底：
  - 如果转换器返回失败且未写入 warning，importer 会补充
    `model conversion failed without diagnostics`，避免静默失败。
- 诊断约定（当前已落地）：
  - `converter.vrm.*`：VRM 转换阶段诊断（source 缺失、header 非法、copy 失败、缓存复用等）。
  - `converter.gltf.*`：GLTF 转换阶段诊断（解析失败、资源缺失、不安全 URI、写入失败、缓存复用等）。
  - `converter.usdz.*` / `converter.fbx.*`：
    - `backend_unavailable`、`backend_unavailable[n]`、`command_template_invalid[n]`、
    - `command_exec_failed[n]`、`command_failed[n]`、
    - `output_missing`、`reuse_cached_canonical`、`exported_canonical` 等。

## 运行时可观测性（新增）
- 来源：`/api/state.mouse_companion_runtime`
- 模型导入链路新增字段：
  - `loaded_model_source_format`：当前成功导入模型的源格式（`glb/gltf/usdz/vrm/fbx/unknown`）。
  - `model_converted_to_canonical`：是否发生了“源格式 -> canonical `.glb`”转换。
  - `model_import_diagnostics`：导入/转换阶段诊断数组（直接透传 converter/importer warning）。
- 设计目标：
  - 多格式接入时，失败定位优先依赖结构化 runtime state，而不是人工翻 host 日志。

## 扩展方式（后续接真实后端）
1. 新增一个 `IModelFormatConverter` 实现（例如 `VrmToGlbConverter`）。
2. 在默认组合管线中注册（按优先级排序）。
3. 保持 `ImportToCanonicalGlb` 接口不变，避免上层（Controller/Runtime）改动。

## 回归基线
- 构建：
  - `cmake --build build-macos --target mfx_entry_runtime_common -j 6`
- 预期：
  - `glb` 仍可直通加载；
  - `vrm` 可直接进入真实转换后端（导出或复用 canonical `.glb`）；
  - `gltf` 可直接进入真实转换后端（导出或复用 canonical `.glb`，并同步依赖资源）；
  - `usdz/fbx` 优先走 tool-backed 真实后端，失败时回落 sidecar；
  - canonical 校验失败时保持 fail-fast。
