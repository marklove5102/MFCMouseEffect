#!/usr/bin/env bash

set -euo pipefail

_mfx_core_http_automation_contract_app_scope_checks() {
    local platform="$1"
    local tmp_dir="$2"
    local base_url="$3"
    local token="$4"

    local code_app_catalog
    code_app_catalog="$(mfx_http_code "$tmp_dir/app-catalog.out" "$base_url/api/automation/app-catalog" -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" -d '{"force":true}')"
    mfx_assert_eq "$code_app_catalog" "200" "core app-catalog status"
    mfx_assert_file_contains "$tmp_dir/app-catalog.out" "\"ok\":true" "core app-catalog ok"
    mfx_assert_file_contains "$tmp_dir/app-catalog.out" "\"count\":" "core app-catalog count field"

    local code_app_catalog_cached
    code_app_catalog_cached="$(mfx_http_code "$tmp_dir/app-catalog-cached.out" "$base_url/api/automation/app-catalog" -X POST -H "x-mfcmouseeffect-token: $token" -H "Content-Type: application/json" -d '{"force":false}')"
    mfx_assert_eq "$code_app_catalog_cached" "200" "core app-catalog cached status"
    mfx_assert_file_contains "$tmp_dir/app-catalog-cached.out" "\"ok\":true" "core app-catalog cached ok"
    mfx_assert_file_contains "$tmp_dir/app-catalog-cached.out" "\"count\":" "core app-catalog cached count field"

    local selected_process
    selected_process="$(_mfx_core_http_automation_first_catalog_process "$tmp_dir/app-catalog.out")"
    if [[ -n "$selected_process" ]]; then
        local selected_scope="process:$selected_process"
        local selected_scope_escaped
        selected_scope_escaped="$(_mfx_core_http_automation_json_escape "$selected_scope")"

        local code_state_apply_scope
        code_state_apply_scope="$(mfx_http_code "$tmp_dir/state-apply-scope.out" "$base_url/api/state" \
            -X POST \
            -H "x-mfcmouseeffect-token: $token" \
            -H "Content-Type: application/json" \
            -d "{\"automation\":{\"enabled\":true,\"mouse_mappings\":[{\"enabled\":true,\"trigger\":\"left_click\",\"app_scopes\":[\"$selected_scope_escaped\"],\"actions\":[{\"type\":\"send_shortcut\",\"shortcut\":\"Cmd+C\"}]}]}}")"
        mfx_assert_eq "$code_state_apply_scope" "200" "core state apply selected-scope status"
        mfx_assert_file_contains "$tmp_dir/state-apply-scope.out" "\"ok\":true" "core state apply selected-scope ok"

        local code_state_after_scope
        code_state_after_scope="$(mfx_http_code "$tmp_dir/state-after-scope.out" "$base_url/api/state" -H "x-mfcmouseeffect-token: $token")"
        mfx_assert_eq "$code_state_after_scope" "200" "core state after selected-scope status"

        local selected_scope_count
        local selected_scope_value
        local selected_legacy_scope_value
        selected_scope_count="$(_mfx_core_http_automation_parse_first_mapping_scope_count "$tmp_dir/state-after-scope.out")"
        selected_scope_value="$(_mfx_core_http_automation_parse_first_mapping_scope_value "$tmp_dir/state-after-scope.out")"
        selected_legacy_scope_value="$(_mfx_core_http_automation_parse_first_mapping_legacy_scope_value "$tmp_dir/state-after-scope.out")"
        mfx_assert_eq "$selected_scope_count" "1" "core selected-scope persisted count"
        mfx_assert_eq "$selected_legacy_scope_value" "$selected_scope_value" "core selected-scope legacy field parity"
        _mfx_core_http_automation_assert_scope_match \
            "$base_url" \
            "$token" \
            "$selected_process" \
            "$selected_scope_value" \
            "true" \
            "$tmp_dir/scope-selected-process-persisted.out" \
            "core selected-scope persisted semantic match"

        if [[ "$platform" != "windows" && "$selected_scope_value" =~ ^process:.*\.(exe|app)$ ]]; then
            mfx_fail "core selected-scope canonicalization on non-windows should not persist suffix token: ${selected_scope_value:-<empty>}"
        fi
    fi

    local code_state_apply_scope_alias_dedupe
    code_state_apply_scope_alias_dedupe="$(mfx_http_code "$tmp_dir/state-apply-scope-alias-dedupe.out" "$base_url/api/state" \
        -X POST \
        -H "x-mfcmouseeffect-token: $token" \
        -H "Content-Type: application/json" \
        -d '{"automation":{"enabled":true,"mouse_mappings":[{"enabled":true,"trigger":"left_click","app_scopes":["process:code.exe","process:code.app","process:code"],"actions":[{"type":"send_shortcut","shortcut":"Cmd+C"}]}]}}')"
    mfx_assert_eq "$code_state_apply_scope_alias_dedupe" "200" "core state apply app-scope alias dedupe status"

    local code_state_after_scope_alias_dedupe
    code_state_after_scope_alias_dedupe="$(mfx_http_code "$tmp_dir/state-after-scope-alias-dedupe.out" "$base_url/api/state" -H "x-mfcmouseeffect-token: $token")"
    mfx_assert_eq "$code_state_after_scope_alias_dedupe" "200" "core state after app-scope alias dedupe status"

    local dedupe_scope_count
    local dedupe_scope_value
    local dedupe_legacy_scope_value
    dedupe_scope_count="$(_mfx_core_http_automation_parse_first_mapping_scope_count "$tmp_dir/state-after-scope-alias-dedupe.out")"
    dedupe_scope_value="$(_mfx_core_http_automation_parse_first_mapping_scope_value "$tmp_dir/state-after-scope-alias-dedupe.out")"
    dedupe_legacy_scope_value="$(_mfx_core_http_automation_parse_first_mapping_legacy_scope_value "$tmp_dir/state-after-scope-alias-dedupe.out")"
    mfx_assert_eq "$dedupe_scope_count" "1" "core app-scope alias dedupe persisted count"
    mfx_assert_eq "$dedupe_legacy_scope_value" "$dedupe_scope_value" "core app-scope alias dedupe legacy field parity"
    local expected_dedupe_scope="process:code.exe"
    if [[ "$platform" != "windows" ]]; then
        expected_dedupe_scope="process:code"
    fi
    mfx_assert_eq "$dedupe_scope_value" "$expected_dedupe_scope" "core app-scope alias dedupe canonical persisted value"

    _mfx_core_http_automation_assert_scope_match "$base_url" "$token" "code" "process:code.exe" "true" "$tmp_dir/scope-code-vs-exe.out" "core app-scope code<->exe"
    _mfx_core_http_automation_assert_scope_match "$base_url" "$token" "code.app" "process:code" "true" "$tmp_dir/scope-app-vs-base.out" "core app-scope app<->base"
    _mfx_core_http_automation_assert_scope_match "$base_url" "$token" "code.exe" "process:code.app" "true" "$tmp_dir/scope-exe-vs-app.out" "core app-scope exe<->app"
    _mfx_core_http_automation_assert_scope_match "$base_url" "$token" "safari" "process:code" "false" "$tmp_dir/scope-negative.out" "core app-scope negative"
    mfx_assert_file_contains "$tmp_dir/scope-code-vs-exe.out" "\"process_aliases\":[\"code\",\"code.exe\",\"code.app\"]" "core app-scope process alias matrix code"
    local expected_scope_code_vs_exe="process:code.exe"
    if [[ "$platform" != "windows" ]]; then
        expected_scope_code_vs_exe="process:code"
    fi
    mfx_assert_file_contains "$tmp_dir/scope-code-vs-exe.out" "\"normalized\":\"$expected_scope_code_vs_exe\"" "core app-scope scope alias normalized token"
    mfx_assert_file_contains "$tmp_dir/scope-code-vs-exe.out" "\"aliases\":[\"code\"" "core app-scope scope alias canonical base"
    mfx_assert_file_contains "$tmp_dir/scope-app-vs-base.out" "\"process_aliases\":[\"code.app\",\"code\"]" "core app-scope process alias matrix code.app"
    mfx_assert_file_contains "$tmp_dir/scope-exe-vs-app.out" "\"process_aliases\":[\"code.exe\",\"code\"]" "core app-scope process alias matrix code.exe"
}
