#!/usr/bin/env bash

set -euo pipefail

_mfx_core_http_automation_contract_match_inject_checks() {
    local tmp_dir="$1"
    local base_url="$2"
    local token="$3"
    local launch_capture_file="$tmp_dir/settings-launch-capture.env"

    local code_match_and_inject
    code_match_and_inject="$(mfx_http_code "$tmp_dir/match-and-inject.out" "$base_url/api/automation/test-match-and-inject" \
        -X POST \
        -H "x-mfcmouseeffect-token: $token" \
        -H "Content-Type: application/json" \
        -d '{"process":"code.app","history":["left_click"],"mappings":[{"enabled":true,"trigger":"left_click","app_scopes":["all"],"actions":[{"type":"delay","delay_ms":1},{"type":"send_shortcut","shortcut":"Cmd+C"}]}]}')"
    mfx_assert_eq "$code_match_and_inject" "200" "core test-match-and-inject status"
    mfx_assert_file_contains "$tmp_dir/match-and-inject.out" "\"ok\":true" "core test-match-and-inject ok"
    mfx_assert_file_contains "$tmp_dir/match-and-inject.out" "\"matched\":true" "core test-match-and-inject matched"
    mfx_assert_file_contains "$tmp_dir/match-and-inject.out" "\"injected\":true" "core test-match-and-inject injected"
    mfx_assert_file_contains "$tmp_dir/match-and-inject.out" "\"selected_shortcut\":\"Cmd+C\"" "core test-match-and-inject selected shortcut"

    rm -f "$launch_capture_file"
    local code_match_and_open_url
    code_match_and_open_url="$(mfx_http_code "$tmp_dir/match-and-open-url.out" "$base_url/api/automation/test-match-and-inject" \
        -X POST \
        -H "x-mfcmouseeffect-token: $token" \
        -H "Content-Type: application/json" \
        -d '{"process":"code.app","history":["left_click"],"mappings":[{"enabled":true,"trigger":"left_click","app_scopes":["all"],"actions":[{"type":"open_url","url":"https://example.com/mfx-automation-contract"}]}]}')"
    mfx_assert_eq "$code_match_and_open_url" "200" "core test-match-and-open-url status"
    mfx_assert_file_contains "$tmp_dir/match-and-open-url.out" "\"ok\":true" "core test-match-and-open-url ok"
    mfx_assert_file_contains "$tmp_dir/match-and-open-url.out" "\"matched\":true" "core test-match-and-open-url matched"
    mfx_assert_file_contains "$tmp_dir/match-and-open-url.out" "\"injected\":true" "core test-match-and-open-url injected"
    mfx_assert_file_contains "$launch_capture_file" "captured=1" "core test-match-and-open-url launch capture flag"
    mfx_assert_file_contains "$launch_capture_file" "target_kind=url" "core test-match-and-open-url launch capture target kind"
    mfx_assert_file_contains "$launch_capture_file" "url=https://example.com/mfx-automation-contract" "core test-match-and-open-url launch capture url"

    rm -f "$launch_capture_file"
    local code_match_and_launch_app
    code_match_and_launch_app="$(mfx_http_code "$tmp_dir/match-and-launch-app.out" "$base_url/api/automation/test-match-and-inject" \
        -X POST \
        -H "x-mfcmouseeffect-token: $token" \
        -H "Content-Type: application/json" \
        -d '{"process":"code.app","history":["left_click"],"mappings":[{"enabled":true,"trigger":"left_click","app_scopes":["all"],"actions":[{"type":"launch_app","app_path":"/Applications/TextEdit.app"}]}]}')"
    mfx_assert_eq "$code_match_and_launch_app" "200" "core test-match-and-launch-app status"
    mfx_assert_file_contains "$tmp_dir/match-and-launch-app.out" "\"ok\":true" "core test-match-and-launch-app ok"
    mfx_assert_file_contains "$tmp_dir/match-and-launch-app.out" "\"matched\":true" "core test-match-and-launch-app matched"
    mfx_assert_file_contains "$tmp_dir/match-and-launch-app.out" "\"injected\":true" "core test-match-and-launch-app injected"
    mfx_assert_file_contains "$launch_capture_file" "captured=1" "core test-match-and-launch-app launch capture flag"
    mfx_assert_file_contains "$launch_capture_file" "target_kind=app" "core test-match-and-launch-app launch capture target kind"
    mfx_assert_file_contains "$launch_capture_file" "app_path=/Applications/TextEdit.app" "core test-match-and-launch-app launch capture path"
}
