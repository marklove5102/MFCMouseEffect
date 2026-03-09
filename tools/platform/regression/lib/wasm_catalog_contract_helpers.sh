#!/usr/bin/env bash

set -euo pipefail

mfx_wasm_catalog_assert_capability_fields() {
    local input_file="$1"
    local context_label="$2"

    python3 - "$input_file" "$context_label" <<'PY'
import json
import sys

input_file = sys.argv[1]
context_label = sys.argv[2]

with open(input_file, "r", encoding="utf-8") as f:
    root = json.load(f)

plugins = root.get("plugins")
errors = root.get("errors")
count = root.get("count")
error_count = root.get("error_count")

if not isinstance(plugins, list):
    print(f"{context_label}: plugins must be array", file=sys.stderr)
    sys.exit(2)
if not isinstance(errors, list):
    print(f"{context_label}: errors must be array", file=sys.stderr)
    sys.exit(3)
if not isinstance(count, int) or count < 0:
    print(f"{context_label}: count must be non-negative integer", file=sys.stderr)
    sys.exit(4)
if not isinstance(error_count, int) or error_count < 0:
    print(f"{context_label}: error_count must be non-negative integer", file=sys.stderr)
    sys.exit(5)
if count != len(plugins):
    print(f"{context_label}: count does not match plugins length", file=sys.stderr)
    sys.exit(6)
if error_count != len(errors):
    print(f"{context_label}: error_count does not match errors length", file=sys.stderr)
    sys.exit(7)

for index, plugin in enumerate(plugins):
    if not isinstance(plugin, dict):
        print(f"{context_label}: plugins[{index}] must be object", file=sys.stderr)
        sys.exit(8)

    if "input_kinds" not in plugin or not isinstance(plugin["input_kinds"], list):
        print(f"{context_label}: plugins[{index}].input_kinds must be array", file=sys.stderr)
        sys.exit(9)
    for kind_index, kind in enumerate(plugin["input_kinds"]):
        if not isinstance(kind, str) or not kind:
            print(
                f"{context_label}: plugins[{index}].input_kinds[{kind_index}] must be non-empty string",
                file=sys.stderr,
            )
            sys.exit(10)

    if "enable_frame_tick" not in plugin or not isinstance(plugin["enable_frame_tick"], bool):
        print(f"{context_label}: plugins[{index}].enable_frame_tick must be bool", file=sys.stderr)
        sys.exit(11)
PY
}

mfx_wasm_catalog_write_negative_fixture() {
    local source_file="$1"
    local output_file="$2"
    local mode="$3"

    python3 - "$source_file" "$output_file" "$mode" <<'PY'
import json
import sys

source_file = sys.argv[1]
output_file = sys.argv[2]
mode = sys.argv[3]

with open(source_file, "r", encoding="utf-8") as f:
    root = json.load(f)

plugins = root.get("plugins")
if not isinstance(plugins, list):
    plugins = []
    root["plugins"] = plugins

if mode in ("drop_input_kinds", "drop_enable_frame_tick") and not plugins:
    plugins.append({
        "id": "fixture.synthetic.plugin",
        "input_kinds": [],
        "enable_frame_tick": False,
    })

if mode == "drop_input_kinds":
    if isinstance(plugins[0], dict):
        plugins[0].pop("input_kinds", None)
elif mode == "drop_enable_frame_tick":
    if isinstance(plugins[0], dict):
        plugins[0].pop("enable_frame_tick", None)
elif mode == "count_mismatch":
    count = root.get("count")
    if not isinstance(count, int) or count < 0:
        count = len(plugins)
    root["count"] = count + 1
else:
    print(f"unsupported mode: {mode}", file=sys.stderr)
    sys.exit(2)

if mode != "count_mismatch":
    root["count"] = len(plugins)

errors = root.get("errors")
if not isinstance(errors, list):
    errors = []
    root["errors"] = errors
root["error_count"] = len(errors)

with open(output_file, "w", encoding="utf-8") as f:
    json.dump(root, f, ensure_ascii=False)
PY
}

mfx_wasm_catalog_assert_capability_fields_rejects() {
    local input_file="$1"
    local context_label="$2"

    if mfx_wasm_catalog_assert_capability_fields "$input_file" "$context_label" >/dev/null 2>&1; then
        mfx_fail "$context_label unexpectedly passed"
    fi
}
