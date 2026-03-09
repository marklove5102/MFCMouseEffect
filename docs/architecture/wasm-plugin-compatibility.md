# WASM Plugin Compatibility Policy

This policy defines forward/backward rules for `MFCMouseEffect` WASM plugins.

## 1. Version markers

- Host contract version: `api_version` in `plugin.json`
- Runtime ABI entry:
  - `mfx_plugin_get_api_version()`
  - currently expected value: `2`

Both values must match host-supported ABI major version.

## 2. ABI compatibility rules

For ABI v2:
- Existing exported function names must stay stable.
- Existing binary struct field order must stay stable.
- New fields are append-only.
- Existing enum values are immutable.
- `mfx_plugin_on_input` and `mfx_plugin_on_frame` are both required.

Breaking layout/function changes require ABI major bump (v3+).

## 3. Manifest compatibility rules

`plugin.json` required fields:
- `id` (stable plugin identity)
- `name`
- `version`
- `api_version`
- `entry` (wasm module relative path)

Optional host-routing fields:
- `input_kinds` (string array, defaults to `all`)
- `enable_frame_tick` (boolean, defaults to `true`)

Rules:
- `id` must remain stable across plugin upgrades.
- `entry` path must stay within plugin folder.
- `input_kinds` narrows host-side input lanes and does not change ABI function signatures.
- `enable_frame_tick=false` means host will not call `mfx_plugin_on_frame`.
- Unknown fields are ignored by host, so plugin authors may add private metadata.

## 4. Runtime fallback behavior

If runtime bridge cannot be loaded:
- host uses `Null` runtime fallback.
- plugin stays non-functional but app remains stable.

If plugin call exceeds budget:
- current event output may be dropped/truncated.
- diagnostics are recorded in `/api/state` `wasm` block.

## 5. Upgrade guidance for plugin authors

- Keep one source branch per ABI major version (`v2`, `v3`).
- Pin template baseline in repository tags/releases.
- Rebuild plugin when host ABI major changes.
- Verify with:
  - `POST /api/wasm/load-manifest`
  - `POST /api/wasm/enable`
  - `GET /api/state` (`wasm.plugin_api_version`, `wasm.last_error`)

## 6. Host-side compatibility commitment (current)

- v2 host accepts ABI v2 plugins only.
- Old ABI v1 plugins are intentionally unsupported (project not released yet).
- Host may add diagnostics and optional manifest metadata without breaking v2 plugins.
