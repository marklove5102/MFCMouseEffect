#!/usr/bin/env bash

set -euo pipefail

_mfx_core_http_wasm_contract_platform_checks() {
    local platform="$1"
    local tmp_dir="$2"

    if [[ "$platform" != "macos" ]]; then
        return 0
    fi

    if [[ ! -f "$tmp_dir/state.out" || ! -f "$tmp_dir/schema.out" ]]; then
        mfx_fail "core wasm platform checks require state/schema snapshots"
    fi

    local state_runtime_backend
    local state_invoke_supported
    local state_render_supported
    local schema_invoke_supported
    local schema_render_supported
    state_runtime_backend="$(_mfx_core_http_read_json_string "$tmp_dir/state.out" "wasm.runtime_backend")"
    state_invoke_supported="$(_mfx_core_http_read_json_bool "$tmp_dir/state.out" "wasm.invoke_supported")"
    state_render_supported="$(_mfx_core_http_read_json_bool "$tmp_dir/state.out" "wasm.render_supported")"
    schema_invoke_supported="$(_mfx_core_http_read_json_bool "$tmp_dir/schema.out" "capabilities.wasm.invoke")"
    schema_render_supported="$(_mfx_core_http_read_json_bool "$tmp_dir/schema.out" "capabilities.wasm.render")"

    mfx_assert_eq "$state_runtime_backend" "wasm3_static" "core wasm runtime backend on macos"
    mfx_assert_eq "$state_invoke_supported" "true" "core wasm invoke capability in state on macos"
    mfx_assert_eq "$state_render_supported" "true" "core wasm render capability in state on macos"
    mfx_assert_eq "$schema_invoke_supported" "true" "core wasm invoke capability in schema on macos"
    mfx_assert_eq "$schema_render_supported" "true" "core wasm render capability in schema on macos"
    mfx_assert_eq "$state_invoke_supported" "$schema_invoke_supported" "core wasm invoke capability schema/state parity in wasm platform checks"
    mfx_assert_eq "$state_render_supported" "$schema_render_supported" "core wasm render capability schema/state parity in wasm platform checks"

    if mfx_file_contains_fixed "$tmp_dir/wasm-catalog.out" "\"wasm_catalog_not_supported_on_this_platform\""; then
        mfx_fail "core wasm catalog support on macos: unexpected unsupported marker"
    fi
    if ! mfx_file_contains_fixed "$tmp_dir/wasm-import-dialog-probe.out" "\"supported\":true"; then
        mfx_fail "core wasm import dialog support on macos: expected supported=true"
    fi
}
