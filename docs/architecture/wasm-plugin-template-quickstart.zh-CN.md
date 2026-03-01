# WASM 插件模板快速开始

模板目录：
- `examples/wasm-plugin-template`

## 1. 构建

```bash
cd examples/wasm-plugin-template
npm install
npm run build
```

或使用 pnpm：

```bash
pnpm install
pnpm run build
```

产物：
- `dist/effect.wasm`
- `dist/plugin.json`

样例预设：

```bash
npm run build:sample -- --sample text-burst
npm run build:samples
```

样例产物：
- `dist/samples/<sample_key>/effect.wasm`
- `dist/samples/<sample_key>/plugin.json`
- `dist/samples/<sample_key>/assets/*`（声明 `image_assets` 时）

## 2. `spawn_image` 资源（可选）

`plugin.json` 可声明 `image_assets`：
- 路径相对 `plugin.json`
- 支持格式：`.png/.jpg/.jpeg/.bmp/.gif/.tif/.tiff`
- `imageId` 通过数组下标映射（越界取模）
- 缺失或非法资源会回退内置图片渲染器

## 3. 放置插件

按 `plugin.json.id` 建目录：
- Debug 默认：`<exe_dir>/plugins/wasm/<plugin_id>/`
- Release 默认：`%AppData%\\MFCMouseEffect\\plugins\\wasm\\<plugin_id>/`
- 额外扫描根：`<exe_dir>/plugins/wasm`（便携兜底）
- Debug 便捷扫描：`examples/wasm-plugin-template/dist`
- 设置页可追加扫描根（`WASM 插件 -> 插件扫描路径`）

至少复制 `effect.wasm` + `plugin.json`；使用 `image_assets` 时要一起复制 `assets/`。

## 4. 加载与启用

```bash
POST /api/wasm/load-manifest
{
  "manifest_path": "C:\\path\\to\\plugins\\wasm\\demo.click.text-rise.v1\\plugin.json"
}
```

```bash
POST /api/wasm/enable
```

## 5. ABI 契约

必需导出：
- `mfx_plugin_on_event(input_ptr, input_len, output_ptr, output_cap)`

建议导出：
- `mfx_plugin_get_api_version() -> 1`
- `mfx_plugin_reset()`

ABI 定义：
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginAbi.h`

## 6. 内置样例 Key

- `text-rise`
- `text-burst`
- `text-spiral`
- `text-wave-chain`
- `image-pulse`
- `image-burst`
- `image-lift`
- `mixed-text-image`
- `mixed-emoji-celebrate`
- `button-adaptive`
- `scroll-particle-burst`

模板详情：
- `examples/wasm-plugin-template/README.md`
- `examples/wasm-plugin-template/README.zh-CN.md`

## 7. 排错

- `load-manifest` 失败：检查 `entry` 与文件路径。
- 无运行时输出：确认 `mfx_wasm_runtime.dll` 是否已构建。
- 找不到运行时桥接：宿主会回退 Null runtime。
- 输出被丢弃：查看 `/api/state` 的 `wasm` 预算/诊断字段。
