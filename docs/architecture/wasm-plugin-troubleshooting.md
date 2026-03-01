# WASM Plugin Troubleshooting

Applies to current v1 route:
- runtime bridge: `mfx_wasm_runtime.dll`
- diagnostics source: `/api/state` -> `wasm`

## 1. `load-manifest` Fails

Symptoms:
- `POST /api/wasm/load-manifest` returns `ok=false`
- `wasm.last_error` mentions manifest/module path

Checks:
- `manifest_path` is absolute and valid
- `plugin.json` contains required fields
- `entry` points to an existing `effect.wasm`

## 2. Runtime Bridge Fails

Symptoms:
- `wasm.plugin_loaded=false`
- `wasm.runtime_backend="null"`
- `wasm.runtime_fallback_reason` is non-empty

Checks:
- build runtime from repo solution (`MFCMouseEffect.slnx`)
- ensure `mfx_wasm_runtime.dll` is beside executable or in search path
- verify exported bridge symbols:
  - `mfx_wasm_runtime_create`
  - `mfx_wasm_runtime_call_on_event`
  - `mfx_wasm_runtime_last_error`

## 3. Plugin Loads but Nothing Renders

Quick fields:
- `wasm.last_output_bytes`
- `wasm.last_command_count`
- `wasm.last_parse_error`
- `wasm.last_rendered_by_wasm`
- `wasm.last_executed_text_commands`
- `wasm.last_executed_image_commands`
- `wasm.last_render_error`
- `wasm.last_budget_reason`

If parser succeeds but still no output:
- check focus/VM suppression policy
- check budget truncation flags

## 4. `spawn_image` Falls Back to Built-in Image

Checks:
- `plugin.json` defines `image_assets`
- paths are relative to `plugin.json`
- file extensions are supported (`png/jpg/jpeg/bmp/gif/tif/tiff`)
- `imageId` maps to expected index

Note: unresolved assets intentionally fallback to built-in renderers.

## 5. Budget Rejected/Truncated

Flags:
- `wasm.last_call_rejected_by_budget`
- `wasm.last_output_truncated_by_budget`
- `wasm.last_command_truncated_by_budget`

Action:
- reduce commands per event
- reduce output bytes
- simplify per-event compute logic

## 6. Parse Errors

Common `wasm.last_parse_error` values:
- `truncated_header`
- `invalid_command_size`
- `truncated_command`
- `unsupported_command_kind`
- `command_limit_exceeded`

Action: align emitted binary layout with `WasmPluginAbi.h` and exact `sizeBytes`.

## 7. Catalog Empty (`No plugins discovered`)

Checks:
- inspect `/api/wasm/catalog` -> `search_roots`
- verify settings `Catalog root path`
- ensure valid `plugin.json` exists under roots
- debug mode auto-scans `examples/wasm-plugin-template/dist`

## 8. Minimal Selfcheck

1. Build template under `examples/wasm-plugin-template`.
2. Place `effect.wasm` + `plugin.json` under plugin folder.
3. Call `/api/wasm/load-manifest` then `/api/wasm/enable`.
4. Click once and inspect `/api/state` `wasm` block.
