#!/usr/bin/env bash

set -euo pipefail

_mfx_core_http_automation_json_escape() {
    local value="$1"
    printf '%s' "$value" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

_mfx_core_http_automation_first_catalog_process() {
    local file_path="$1"
    sed -n 's/.*"exe":"\([^"]*\)".*/\1/p' "$file_path" | head -n 1
}

_mfx_core_http_automation_parse_uint_field() {
    local input_file="$1"
    local field_name="$2"
    sed -n "s/.*\"${field_name}\":\\([0-9][0-9]*\\).*/\\1/p" "$input_file" | head -n 1
}

_mfx_core_http_automation_parse_scalar_field() {
    local input_file="$1"
    local field_name="$2"
    sed -n "s/.*\"${field_name}\":[[:space:]]*\\([^,}]*\\).*/\\1/p" "$input_file" | head -n 1 | tr -d '[:space:]'
}

_mfx_core_http_automation_parse_active_field() {
    local input_file="$1"
    local field_name="$2"
    tr -d '\n\r' <"$input_file" | \
        sed -n "s/.*\"active\":{[^}]*\"${field_name}\":\"\\([^\"]*\\)\"[^}]*}.*/\\1/p" | \
        head -n 1
}

_mfx_core_http_automation_parse_section_scalar_field() {
    local input_file="$1"
    local section_name="$2"
    local field_name="$3"
    tr -d '\n\r' <"$input_file" | \
        sed -n "s/.*\"${section_name}\":{[^}]*\"${field_name}\":[[:space:]]*\\([^,}]*\\).*/\\1/p" | \
        head -n 1 | tr -d '[:space:]'
}

_mfx_core_http_automation_parse_nested_section_scalar_field() {
    local input_file="$1"
    local section_name="$2"
    local nested_section_name="$3"
    local field_name="$4"
    : "$section_name"
    tr -d '\n\r' <"$input_file" | \
        sed -n "s/.*\"${nested_section_name}\":{[^}]*\"${field_name}\":[[:space:]]*\\([^,}]*\\).*/\\1/p" | \
        head -n 1 | tr -d '[:space:]'
}

_mfx_core_http_automation_parse_first_mapping_scope_count() {
    local input_file="$1"
    python3 - "$input_file" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as f:
    root = json.load(f)

mappings = (
    root.get("automation", {})
        .get("mouse_mappings", [])
)
if not mappings:
    print("0")
    sys.exit(0)

scopes = mappings[0].get("app_scopes", [])
if not isinstance(scopes, list):
    print("0")
    sys.exit(0)

print(len(scopes))
PY
}

_mfx_core_http_automation_parse_first_mapping_scope_value() {
    local input_file="$1"
    python3 - "$input_file" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as f:
    root = json.load(f)

mappings = (
    root.get("automation", {})
        .get("mouse_mappings", [])
)
if not mappings:
    print("")
    sys.exit(0)

scopes = mappings[0].get("app_scopes", [])
if not isinstance(scopes, list) or not scopes:
    print("")
    sys.exit(0)

value = scopes[0]
print(value if isinstance(value, str) else "")
PY
}

_mfx_core_http_automation_parse_first_mapping_legacy_scope_value() {
    local input_file="$1"
    python3 - "$input_file" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as f:
    root = json.load(f)

mappings = (
    root.get("automation", {})
        .get("mouse_mappings", [])
)
if not mappings:
    print("")
    sys.exit(0)

value = mappings[0].get("app_scope", "")
print(value if isinstance(value, str) else "")
PY
}
