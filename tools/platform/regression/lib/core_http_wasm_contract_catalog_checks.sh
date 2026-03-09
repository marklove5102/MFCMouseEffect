#!/usr/bin/env bash

set -euo pipefail

_mfx_core_http_wasm_contract_catalog_checks_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$_mfx_core_http_wasm_contract_catalog_checks_dir/wasm_catalog_contract_helpers.sh"

_mfx_core_http_wasm_catalog_assert_capability_fields() {
    local input_file="$1"
    local context_label="$2"
    mfx_wasm_catalog_assert_capability_fields "$input_file" "$context_label"
}

_mfx_core_http_wasm_catalog_write_negative_fixture() {
    local source_file="$1"
    local output_file="$2"
    local mode="$3"
    mfx_wasm_catalog_write_negative_fixture "$source_file" "$output_file" "$mode"
}

_mfx_core_http_wasm_catalog_assert_capability_fields_rejects() {
    local input_file="$1"
    local context_label="$2"
    mfx_wasm_catalog_assert_capability_fields_rejects "$input_file" "$context_label"
}

_mfx_core_http_wasm_catalog_first_plugin_field() {
    local input_file="$1"
    local field_name="$2"

    python3 - "$input_file" "$field_name" <<'PY'
import json
import sys

input_file = sys.argv[1]
field_name = sys.argv[2]

with open(input_file, "r", encoding="utf-8") as f:
    root = json.load(f)

plugins = root.get("plugins")
if not isinstance(plugins, list) or not plugins:
    sys.exit(0)

value = plugins[0].get(field_name)
if isinstance(value, str):
    sys.stdout.write(value)
PY
}

_mfx_core_http_wasm_catalog_assert_plugin_id_count() {
    local input_file="$1"
    local plugin_id="$2"
    local expected_count="$3"
    local context_label="$4"

    python3 - "$input_file" "$plugin_id" "$expected_count" "$context_label" <<'PY'
import json
import sys

input_file = sys.argv[1]
plugin_id = sys.argv[2].strip().lower()
expected_count = int(sys.argv[3])
context_label = sys.argv[4]

with open(input_file, "r", encoding="utf-8") as f:
    root = json.load(f)

plugins = root.get("plugins")
if not isinstance(plugins, list):
    print(f"{context_label}: plugins must be array", file=sys.stderr)
    sys.exit(2)

count = 0
for plugin in plugins:
    if isinstance(plugin, dict) and str(plugin.get("id", "")).strip().lower() == plugin_id:
        count += 1

if count != expected_count:
    print(f"{context_label}: expected {expected_count}, got {count}", file=sys.stderr)
    sys.exit(3)
PY
}

_mfx_core_http_wasm_catalog_assert_plugin_manifest_path() {
    local input_file="$1"
    local plugin_id="$2"
    local expected_manifest_path="$3"
    local context_label="$4"

    python3 - "$input_file" "$plugin_id" "$expected_manifest_path" "$context_label" <<'PY'
import json
import os
import sys

input_file = sys.argv[1]
plugin_id = sys.argv[2].strip().lower()
expected_manifest_path = os.path.realpath(sys.argv[3]).replace("\\", "/").lower()
context_label = sys.argv[4]

with open(input_file, "r", encoding="utf-8") as f:
    root = json.load(f)

plugins = root.get("plugins")
if not isinstance(plugins, list):
    print(f"{context_label}: plugins must be array", file=sys.stderr)
    sys.exit(2)

for plugin in plugins:
    if not isinstance(plugin, dict):
        continue
    if str(plugin.get("id", "")).strip().lower() != plugin_id:
        continue
    actual_path = os.path.realpath(str(plugin.get("manifest_path", ""))).replace("\\", "/").lower()
    if actual_path != expected_manifest_path:
        print(f"{context_label}: expected {expected_manifest_path}, got {actual_path}", file=sys.stderr)
        sys.exit(3)
    sys.exit(0)

print(f"{context_label}: plugin id not found", file=sys.stderr)
sys.exit(4)
PY
}

_mfx_core_http_wasm_catalog_assert_no_duplicate_error() {
    local input_file="$1"
    local context_label="$2"

    if mfx_file_contains_fixed "$input_file" "Duplicated plugin id"; then
        mfx_fail "$context_label: duplicated plugin id error should not appear"
    fi
}

_mfx_core_http_wasm_catalog_write_policy_payload() {
    local output_file="$1"
    local catalog_root_path="$2"

    python3 - "$output_file" "$catalog_root_path" <<'PY'
import json
import sys

output_file = sys.argv[1]
catalog_root_path = sys.argv[2]

with open(output_file, "w", encoding="utf-8") as f:
    json.dump({"catalog_root_path": catalog_root_path}, f, ensure_ascii=False)
PY
}

_mfx_core_http_wasm_catalog_rewrite_manifest_fields() {
    local manifest_path="$1"
    local plugin_id="$2"
    local plugin_name="$3"
    local api_version="$4"

    python3 - "$manifest_path" "$plugin_id" "$plugin_name" "$api_version" <<'PY'
import json
import sys

manifest_path = sys.argv[1]
plugin_id = sys.argv[2]
plugin_name = sys.argv[3]
api_version = sys.argv[4]

with open(manifest_path, "r", encoding="utf-8") as f:
    root = json.load(f)

root["id"] = plugin_id
root["name"] = plugin_name
root["api_version"] = int(api_version)

with open(manifest_path, "w", encoding="utf-8") as f:
    json.dump(root, f, ensure_ascii=False, indent=2)
    f.write("\n")
PY
}

_mfx_core_http_wasm_contract_catalog_checks() {
    local tmp_dir="$1"
    local base_url="$2"
    local token="$3"
    local repo_root="$4"

    local code_wasm_catalog
    code_wasm_catalog="$(mfx_http_code "$tmp_dir/wasm-catalog.out" "$base_url/api/wasm/catalog" -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" -d '{}')"
    mfx_assert_eq "$code_wasm_catalog" "200" "core wasm catalog status"
    mfx_assert_file_contains "$tmp_dir/wasm-catalog.out" "\"ok\":true" "core wasm catalog ok"
    mfx_assert_file_contains "$tmp_dir/wasm-catalog.out" "\"plugins\":" "core wasm catalog plugins field"
    mfx_assert_file_contains "$tmp_dir/wasm-catalog.out" "\"search_roots\":" "core wasm catalog search_roots field"
    _mfx_core_http_wasm_catalog_assert_capability_fields \
        "$tmp_dir/wasm-catalog.out" \
        "core wasm catalog capability fields"
    _mfx_core_http_wasm_catalog_write_negative_fixture \
        "$tmp_dir/wasm-catalog.out" \
        "$tmp_dir/wasm-catalog-fixture-missing-input-kinds.out" \
        "drop_input_kinds"
    _mfx_core_http_wasm_catalog_assert_capability_fields_rejects \
        "$tmp_dir/wasm-catalog-fixture-missing-input-kinds.out" \
        "core wasm catalog negative fixture missing input_kinds"
    _mfx_core_http_wasm_catalog_write_negative_fixture \
        "$tmp_dir/wasm-catalog.out" \
        "$tmp_dir/wasm-catalog-fixture-missing-enable-frame-tick.out" \
        "drop_enable_frame_tick"
    _mfx_core_http_wasm_catalog_assert_capability_fields_rejects \
        "$tmp_dir/wasm-catalog-fixture-missing-enable-frame-tick.out" \
        "core wasm catalog negative fixture missing enable_frame_tick"
    _mfx_core_http_wasm_catalog_write_negative_fixture \
        "$tmp_dir/wasm-catalog.out" \
        "$tmp_dir/wasm-catalog-fixture-count-mismatch.out" \
        "count_mismatch"
    _mfx_core_http_wasm_catalog_assert_capability_fields_rejects \
        "$tmp_dir/wasm-catalog-fixture-count-mismatch.out" \
        "core wasm catalog negative fixture count mismatch"

    local original_catalog_root_path
    original_catalog_root_path="$(_mfx_core_http_read_json_string "$tmp_dir/state.out" "wasm.configured_catalog_root_path")"
    local duplicate_source_manifest_path
    duplicate_source_manifest_path="$(_mfx_core_http_wasm_catalog_first_plugin_field "$tmp_dir/wasm-catalog.out" "manifest_path")"
    local duplicate_plugin_id
    duplicate_plugin_id="$(_mfx_core_http_wasm_catalog_first_plugin_field "$tmp_dir/wasm-catalog.out" "id")"
    if [[ -n "$duplicate_source_manifest_path" && -n "$duplicate_plugin_id" ]]; then
        local duplicate_root="$tmp_dir/wasm-catalog-duplicate-root"
        local duplicate_plugin_dir="$duplicate_root/duplicate-copy"
        local duplicate_manifest_path="$duplicate_plugin_dir/plugin.json"
        mkdir -p "$duplicate_root"
        cp -R "$(dirname "$duplicate_source_manifest_path")" "$duplicate_plugin_dir"

        local duplicate_catalog_policy_payload="$tmp_dir/wasm-catalog-duplicate-policy.json"
        _mfx_core_http_wasm_catalog_write_policy_payload "$duplicate_catalog_policy_payload" "$duplicate_root"
        local code_apply_duplicate_catalog_root
        code_apply_duplicate_catalog_root="$(mfx_http_code "$tmp_dir/wasm-policy-duplicate-root.out" "$base_url/api/wasm/policy" \
            -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" --data-binary "@$duplicate_catalog_policy_payload")"
        mfx_assert_eq "$code_apply_duplicate_catalog_root" "200" "core wasm duplicate catalog root apply status"
        mfx_assert_file_contains "$tmp_dir/wasm-policy-duplicate-root.out" "\"ok\":true" "core wasm duplicate catalog root apply ok"

        local code_wasm_catalog_with_duplicate_root
        code_wasm_catalog_with_duplicate_root="$(mfx_http_code "$tmp_dir/wasm-catalog-duplicate-root.out" "$base_url/api/wasm/catalog" -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" -d '{}')"
        mfx_assert_eq "$code_wasm_catalog_with_duplicate_root" "200" "core wasm catalog duplicate-root status"
        _mfx_core_http_wasm_catalog_assert_capability_fields \
            "$tmp_dir/wasm-catalog-duplicate-root.out" \
            "core wasm catalog duplicate-root capability fields"
        _mfx_core_http_wasm_catalog_assert_plugin_id_count \
            "$tmp_dir/wasm-catalog-duplicate-root.out" \
            "$duplicate_plugin_id" \
            "1" \
            "core wasm catalog duplicate-root id count"
        _mfx_core_http_wasm_catalog_assert_plugin_manifest_path \
            "$tmp_dir/wasm-catalog-duplicate-root.out" \
            "$duplicate_plugin_id" \
            "$duplicate_manifest_path" \
            "core wasm catalog duplicate-root precedence"
        _mfx_core_http_wasm_catalog_assert_no_duplicate_error \
            "$tmp_dir/wasm-catalog-duplicate-root.out" \
            "core wasm catalog duplicate-root dedupe"

        local restore_catalog_policy_payload="$tmp_dir/wasm-catalog-restore-policy.json"
        _mfx_core_http_wasm_catalog_write_policy_payload "$restore_catalog_policy_payload" "$original_catalog_root_path"
        local code_restore_catalog_root
        code_restore_catalog_root="$(mfx_http_code "$tmp_dir/wasm-policy-restore-root.out" "$base_url/api/wasm/policy" \
            -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" --data-binary "@$restore_catalog_policy_payload")"
        mfx_assert_eq "$code_restore_catalog_root" "200" "core wasm duplicate catalog root restore status"
        mfx_assert_file_contains "$tmp_dir/wasm-policy-restore-root.out" "\"ok\":true" "core wasm duplicate catalog root restore ok"

        local legacy_nested_root="$tmp_dir/wasm-catalog-legacy-nested-root"
        local parent_plugin_dir="$legacy_nested_root/fixture-parent"
        local nested_plugin_dir="$parent_plugin_dir/samples/nested"
        local legacy_plugin_dir="$legacy_nested_root/fixture-legacy"
        mkdir -p "$parent_plugin_dir" "$nested_plugin_dir" "$legacy_plugin_dir"
        cp -R "$(dirname "$duplicate_source_manifest_path")"/. "$parent_plugin_dir"
        cp -R "$(dirname "$duplicate_source_manifest_path")"/. "$nested_plugin_dir"
        cp -R "$(dirname "$duplicate_source_manifest_path")"/. "$legacy_plugin_dir"

        _mfx_core_http_wasm_catalog_rewrite_manifest_fields \
            "$parent_plugin_dir/plugin.json" \
            "fixture.catalog.parent.v2" \
            "Fixture Catalog Parent" \
            "2"
        _mfx_core_http_wasm_catalog_rewrite_manifest_fields \
            "$nested_plugin_dir/plugin.json" \
            "fixture.catalog.nested.v2" \
            "Fixture Catalog Nested" \
            "2"
        _mfx_core_http_wasm_catalog_rewrite_manifest_fields \
            "$legacy_plugin_dir/plugin.json" \
            "fixture.catalog.legacy.v1" \
            "Fixture Catalog Legacy" \
            "1"

        local legacy_nested_catalog_policy_payload="$tmp_dir/wasm-catalog-legacy-nested-policy.json"
        _mfx_core_http_wasm_catalog_write_policy_payload "$legacy_nested_catalog_policy_payload" "$legacy_nested_root"
        local code_apply_legacy_nested_catalog_root
        code_apply_legacy_nested_catalog_root="$(mfx_http_code "$tmp_dir/wasm-policy-legacy-nested-root.out" "$base_url/api/wasm/policy" \
            -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" --data-binary "@$legacy_nested_catalog_policy_payload")"
        mfx_assert_eq "$code_apply_legacy_nested_catalog_root" "200" "core wasm legacy+nested catalog root apply status"
        mfx_assert_file_contains "$tmp_dir/wasm-policy-legacy-nested-root.out" "\"ok\":true" "core wasm legacy+nested catalog root apply ok"

        local code_wasm_catalog_legacy_nested_root
        code_wasm_catalog_legacy_nested_root="$(mfx_http_code "$tmp_dir/wasm-catalog-legacy-nested-root.out" "$base_url/api/wasm/catalog" -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" -d '{}')"
        mfx_assert_eq "$code_wasm_catalog_legacy_nested_root" "200" "core wasm catalog legacy+nested root status"
        _mfx_core_http_wasm_catalog_assert_capability_fields \
            "$tmp_dir/wasm-catalog-legacy-nested-root.out" \
            "core wasm catalog legacy+nested capability fields"
        _mfx_core_http_wasm_catalog_assert_plugin_id_count \
            "$tmp_dir/wasm-catalog-legacy-nested-root.out" \
            "fixture.catalog.parent.v2" \
            "1" \
            "core wasm catalog parent plugin retained"
        _mfx_core_http_wasm_catalog_assert_plugin_id_count \
            "$tmp_dir/wasm-catalog-legacy-nested-root.out" \
            "fixture.catalog.nested.v2" \
            "0" \
            "core wasm catalog nested plugin skipped"
        _mfx_core_http_wasm_catalog_assert_plugin_id_count \
            "$tmp_dir/wasm-catalog-legacy-nested-root.out" \
            "fixture.catalog.legacy.v1" \
            "0" \
            "core wasm catalog legacy plugin skipped"

        _mfx_core_http_wasm_catalog_write_policy_payload "$restore_catalog_policy_payload" "$original_catalog_root_path"
        code_restore_catalog_root="$(mfx_http_code "$tmp_dir/wasm-policy-restore-root.out" "$base_url/api/wasm/policy" \
            -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" --data-binary "@$restore_catalog_policy_payload")"
        mfx_assert_eq "$code_restore_catalog_root" "200" "core wasm legacy+nested catalog root restore status"
        mfx_assert_file_contains "$tmp_dir/wasm-policy-restore-root.out" "\"ok\":true" "core wasm legacy+nested catalog root restore ok"
    fi

    _mfx_core_http_assert_wasm_import_dialog_probe_supported \
        "$tmp_dir/wasm-import-dialog-probe.out" \
        "$base_url" \
        "$token" \
        "core wasm import dialog probe"

    _mfx_core_http_assert_wasm_import_dialog_probe_trimmed_initial_path \
        "$tmp_dir/wasm-import-dialog-probe-trimmed-initial-path.out" \
        "$base_url" \
        "$token" \
        "$repo_root/examples" \
        "core wasm import dialog probe trimmed initial path"
}
