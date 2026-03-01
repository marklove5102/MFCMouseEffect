#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/../../.." && pwd)"

source "$repo_root/tools/platform/regression/lib/common.sh"
source "$repo_root/tools/platform/regression/lib/build.sh"
source "$repo_root/tools/platform/manual/lib/macos_core_host.sh"

usage() {
    cat <<'EOF'
Usage:
  tools/platform/manual/run-macos-effects-type-parity-selfcheck.sh [options]

Options:
  --build-dir <path>          Build directory (default: /tmp/mfx-platform-macos-core-build)
  --log-file <path>           Host log path (default: /tmp/mfx-core-effects-type-parity.log)
  --probe-file <path>         Probe file path (default: /tmp/mfx-core-effects-type-parity.probe)
  --jobs <num>                Build jobs (sets MFX_BUILD_JOBS)
  --skip-build                Skip cmake configure/build
  --keep-running              Keep host running after selfcheck
  --auto-stop-seconds <num>   Auto stop host after N seconds (only with --keep-running, default: 120)
  -h, --help                  Show this help
EOF
}

build_dir="/tmp/mfx-platform-macos-core-build"
log_file="/tmp/mfx-core-effects-type-parity.log"
probe_file="/tmp/mfx-core-effects-type-parity.probe"
skip_build=0
keep_running=0
auto_stop_seconds=120
build_jobs=""

while [[ $# -gt 0 ]]; do
    case "$1" in
    --build-dir)
        [[ $# -ge 2 ]] || mfx_fail "missing value for --build-dir"
        build_dir="$2"
        shift 2
        ;;
    --log-file)
        [[ $# -ge 2 ]] || mfx_fail "missing value for --log-file"
        log_file="$2"
        shift 2
        ;;
    --probe-file)
        [[ $# -ge 2 ]] || mfx_fail "missing value for --probe-file"
        probe_file="$2"
        shift 2
        ;;
    --jobs)
        [[ $# -ge 2 ]] || mfx_fail "missing value for --jobs"
        build_jobs="$2"
        shift 2
        ;;
    --skip-build)
        skip_build=1
        shift
        ;;
    --keep-running)
        keep_running=1
        shift
        ;;
    --auto-stop-seconds)
        [[ $# -ge 2 ]] || mfx_fail "missing value for --auto-stop-seconds"
        auto_stop_seconds="$2"
        shift 2
        ;;
    -h|--help)
        usage
        exit 0
        ;;
    *)
        mfx_fail "unknown argument: $1"
        ;;
    esac
done

if [[ "$OSTYPE" != darwin* ]]; then
    mfx_fail "this script is macOS-only"
fi

mfx_manual_apply_build_jobs_env "$build_jobs" "--jobs"
mfx_manual_validate_non_negative_integer "$auto_stop_seconds" "--auto-stop-seconds"
mfx_require_cmd curl

tmp_dir="$(mktemp -d)"
cleanup() {
    rm -rf "$tmp_dir" >/dev/null 2>&1 || true
    mfx_release_lock
    if [[ "$keep_running" -eq 0 ]]; then
        mfx_manual_stop_core_host "$MFX_MANUAL_HOST_PID"
    fi
}
trap cleanup EXIT

mfx_manual_acquire_entry_host_lock
mfx_manual_prepare_core_host_binary "$repo_root" "$build_dir" "$skip_build"
host_bin="$MFX_MANUAL_HOST_BIN"

declare -a host_env
host_env+=("MFX_ENABLE_EFFECT_OVERLAY_TEST_API=1")
mfx_manual_start_core_host "$host_bin" "$probe_file" "$log_file" "${host_env[@]}"

printf 'mfx_pid=%s\n' "$MFX_MANUAL_HOST_PID"
printf 'settings_url=%s\n' "$MFX_MANUAL_SETTINGS_URL"
printf 'log_file=%s\n' "$MFX_MANUAL_LOG_FILE"

token="$MFX_MANUAL_SETTINGS_TOKEN"
token_header="x-mfcmouseeffect-token: $token"

profile_probe_file="$tmp_dir/effects-profile-probe.out"
profile_code="$(mfx_http_code "$profile_probe_file" "$MFX_MANUAL_BASE_URL/api/effects/test-render-profiles" \
    -X GET \
    -H "$token_header")"
mfx_assert_eq "$profile_code" "200" "effects profile probe status"
mfx_assert_file_contains "$profile_probe_file" "\"ok\":true" "effects profile probe ok"
mfx_assert_file_contains "$profile_probe_file" "\"alias_matrix\":" "effects profile alias matrix section"
mfx_assert_file_contains "$profile_probe_file" "\"active_normalized\":" "effects profile active normalized sample input"
mfx_assert_file_contains "$profile_probe_file" "\"input\":\"scifi\",\"normalized\":\"tubes\"" "effects profile trail scifi alias"
mfx_assert_file_contains "$profile_probe_file" "\"input\":\"stardust\",\"normalized\":\"twinkle\"" "effects profile scroll stardust alias"
mfx_assert_file_contains "$profile_probe_file" "\"input\":\"suspension\",\"normalized\":\"tubes\"" "effects profile hover suspension alias"

state_apply_file="$tmp_dir/state-apply.out"
state_apply_code="$(mfx_http_code "$state_apply_file" "$MFX_MANUAL_BASE_URL/api/state" \
    -X POST \
    -H "$token_header" \
    -H "Content-Type: application/json" \
    -d '{"active":{"click":"text","trail":"meteor","scroll":"helix","hold":"hologram","hover":"tubes"},"hold_follow_mode":"cursor_priority"}')"
mfx_assert_eq "$state_apply_code" "200" "effects state apply status"
mfx_assert_file_contains "$state_apply_file" "\"ok\":true" "effects state apply ok"

state_after_file="$tmp_dir/state-after.out"
state_after_code="$(mfx_http_code "$state_after_file" "$MFX_MANUAL_BASE_URL/api/state" \
    -X GET \
    -H "$token_header")"
mfx_assert_eq "$state_after_code" "200" "effects state after status"
mfx_assert_file_contains "$state_after_file" "\"click\":\"text\"" "effects state click active"
mfx_assert_file_contains "$state_after_file" "\"trail\":\"meteor\"" "effects state trail active"
mfx_assert_file_contains "$state_after_file" "\"scroll\":\"helix\"" "effects state scroll active"
mfx_assert_file_contains "$state_after_file" "\"hold\":\"hologram\"" "effects state hold active"
mfx_assert_file_contains "$state_after_file" "\"hover\":\"tubes\"" "effects state hover active"
mfx_assert_file_contains "$state_after_file" "\"hold_follow_mode\":\"smooth\"" "effects state hold follow mode alias normalization"

overlay_probe_file="$tmp_dir/effects-overlay-probe.out"
overlay_probe_code="$(mfx_http_code "$overlay_probe_file" "$MFX_MANUAL_BASE_URL/api/effects/test-overlay-windows" \
    -X POST \
    -H "$token_header" \
    -H "Content-Type: application/json" \
    -d '{"emit_click":true,"emit_trail":true,"emit_scroll":true,"emit_hold":true,"emit_hover":true,"click_type":"text","trail_type":"meteor","scroll_type":"helix","hold_type":"hologram","hover_type":"tubes","close_persistent":true,"wait_ms":80,"wait_for_clear_ms":1600}')"
mfx_assert_eq "$overlay_probe_code" "200" "effects overlay probe status"
mfx_assert_file_contains "$overlay_probe_file" "\"ok\":true" "effects overlay probe ok"
mfx_assert_file_contains "$overlay_probe_file" "\"click_type\":\"text\"" "effects overlay click type"
mfx_assert_file_contains "$overlay_probe_file" "\"trail_type\":\"meteor\"" "effects overlay trail type"
mfx_assert_file_contains "$overlay_probe_file" "\"scroll_type\":\"helix\"" "effects overlay scroll type"
mfx_assert_file_contains "$overlay_probe_file" "\"hold_type\":\"hologram\"" "effects overlay hold type"
mfx_assert_file_contains "$overlay_probe_file" "\"hover_type\":\"tubes\"" "effects overlay hover type"

overlay_trail_none_probe_file="$tmp_dir/effects-overlay-trail-none-probe.out"
overlay_trail_none_probe_code="$(mfx_http_code "$overlay_trail_none_probe_file" "$MFX_MANUAL_BASE_URL/api/effects/test-overlay-windows" \
    -X POST \
    -H "$token_header" \
    -H "Content-Type: application/json" \
    -d '{"emit_trail":true,"trail_type":"none","close_persistent":true,"reset_line_trail":true,"wait_ms":40,"wait_for_clear_ms":400}')"
mfx_assert_eq "$overlay_trail_none_probe_code" "200" "effects overlay trail none probe status"
mfx_assert_file_contains "$overlay_trail_none_probe_file" "\"ok\":true" "effects overlay trail none probe ok"
mfx_assert_file_contains "$overlay_trail_none_probe_file" "\"trail_type\":\"none\"" "effects overlay trail none probe trail type"
mfx_assert_file_contains "$overlay_trail_none_probe_file" "\"before_line_trail_active\":false" "effects overlay trail none probe before inactive"
mfx_assert_file_contains "$overlay_trail_none_probe_file" "\"after_line_trail_active\":false" "effects overlay trail none probe after inactive"
mfx_assert_file_contains "$overlay_trail_none_probe_file" "\"before_line_trail_point_count\":0" "effects overlay trail none probe before point count"
mfx_assert_file_contains "$overlay_trail_none_probe_file" "\"after_line_trail_point_count\":0" "effects overlay trail none probe after point count"

mfx_ok "macos effects type parity selfcheck passed"

if [[ "$keep_running" -eq 1 ]]; then
    if [[ "$auto_stop_seconds" -gt 0 ]]; then
        mfx_manual_schedule_auto_stop "$MFX_MANUAL_HOST_PID" "$auto_stop_seconds"
        printf 'auto_stop_seconds=%s\n' "$auto_stop_seconds"
    fi
    mfx_manual_print_stop_command "$MFX_MANUAL_HOST_PID"
fi
