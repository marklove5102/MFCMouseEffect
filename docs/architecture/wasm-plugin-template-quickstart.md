# WASM Plugin Template Quick Start

Template root:
- `examples/wasm-plugin-template`

## 1. Build

```bash
cd examples/wasm-plugin-template
npm install
npm run build
```

Or use pnpm:

```bash
pnpm install
pnpm run build
```

Output:
- `dist/effect.wasm`
- `dist/plugin.json`

Sample presets:

```bash
npm run build:sample -- --sample text-burst
npm run build:samples
```

Preset output:
- `dist/samples/<sample_key>/effect.wasm`
- `dist/samples/<sample_key>/plugin.json`
- `dist/samples/<sample_key>/assets/*` (if `image_assets` is declared)

## 2. Optional `spawn_image` Assets

`plugin.json` can include `image_assets`.
- paths are relative to `plugin.json`
- supported: `.png/.jpg/.jpeg/.bmp/.gif/.tif/.tiff`
- `imageId` maps by index (modulo)
- invalid/missing assets fallback to built-in image renderer

## 3. Place Plugin

Create folder by `plugin.json.id`:
- debug default: `<exe_dir>/plugins/wasm/<plugin_id>/`
- release default: `%AppData%\\MFCMouseEffect\\plugins\\wasm\\<plugin_id>/`
- extra scan root: `<exe_dir>/plugins/wasm` (portable fallback)
- debug convenience: host also scans `examples/wasm-plugin-template/dist`
- settings page can append an extra catalog root (`WASM Plugin -> Catalog root path`)

Copy at least `effect.wasm` + `plugin.json`; copy `assets/` when `image_assets` is used.

## 4. Load + Enable

```bash
POST /api/wasm/load-manifest
{
  "manifest_path": "C:\\path\\to\\plugins\\wasm\\demo.click.text-rise.v1\\plugin.json"
}
```

```bash
POST /api/wasm/enable
```

## 5. ABI Contract

Required export:
- `mfx_plugin_on_event(input_ptr, input_len, output_ptr, output_cap)`

Recommended exports:
- `mfx_plugin_get_api_version() -> 1`
- `mfx_plugin_reset()`

ABI definition:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Wasm/WasmPluginAbi.h`

## 6. Built-in Presets

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

Template details:
- `examples/wasm-plugin-template/README.md`
- `examples/wasm-plugin-template/README.zh-CN.md`

## 7. Troubleshooting

- `load-manifest` fails: check `entry` and file existence.
- No runtime output: verify `mfx_wasm_runtime.dll` build output.
- Runtime bridge missing: host falls back to Null runtime.
- Output dropped: inspect `/api/state` -> `wasm` diagnostics/budget flags.
