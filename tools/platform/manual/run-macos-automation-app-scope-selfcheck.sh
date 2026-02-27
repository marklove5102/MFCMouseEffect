#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/../../.." && pwd)"

source "$repo_root/tools/platform/regression/lib/common.sh"
source "$repo_root/tools/platform/regression/lib/build.sh"
source "$repo_root/tools/platform/regression/lib/core_http_automation_parse_helpers.sh"
source "$repo_root/tools/platform/regression/lib/core_http_automation_assert_helpers.sh"
source "$repo_root/tools/platform/manual/lib/macos_core_host.sh"

usage() {
    cat <<'EOF'
Usage:
  tools/platform/manual/run-macos-automation-app-scope-selfcheck.sh [options]

Options:
  --build-dir <path>          Build directory (default: /tmp/mfx-platform-macos-core-build)
  --log-file <path>           Host log path (default: /tmp/mfx-core-automation-app-scope-selfcheck.log)
  --probe-file <path>         Probe file path (default: /tmp/mfx-core-automation-app-scope-selfcheck.probe)
  --jobs <num>                Build jobs (sets MFX_BUILD_JOBS)
  --skip-build                Skip cmake configure/build
  --keep-running              Keep host running after selfcheck
  --auto-stop-seconds <num>   Auto stop host after N seconds (only used with --keep-running, default: 120)
  -h, --help                  Show this help
EOF
}

build_dir="/tmp/mfx-platform-macos-core-build"
log_file="/tmp/mfx-core-automation-app-scope-selfcheck.log"
probe_file="/tmp/mfx-core-automation-app-scope-selfcheck.probe"
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

if [[ -n "$build_jobs" ]]; then
    if ! [[ "$build_jobs" =~ ^[0-9]+$ ]]; then
        mfx_fail "--jobs must be a positive integer"
    fi
    export MFX_BUILD_JOBS="$build_jobs"
fi

if ! [[ "$auto_stop_seconds" =~ ^[0-9]+$ ]]; then
    mfx_fail "--auto-stop-seconds must be a non-negative integer"
fi

mfx_require_cmd cmake
mfx_require_cmd curl

if [[ "$skip_build" -eq 0 ]]; then
    mfx_configure_platform_build_dir "$repo_root" "$build_dir" "macos" \
        -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON \
        -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON \
        -DMFX_ENABLE_POSIX_CORE_RUNTIME=ON
    mfx_build_targets "$build_dir" "mfx_entry_posix_host"
fi

host_bin="$build_dir/mfx_entry_posix_host"
if [[ ! -x "$host_bin" ]]; then
    mfx_fail "host binary missing or not executable: $host_bin"
fi

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

declare -a host_env
host_env+=("MFX_ENABLE_AUTOMATION_SCOPE_TEST_API=1")
mfx_manual_start_core_host "$host_bin" "$probe_file" "$log_file" "${host_env[@]}"

printf 'mfx_pid=%s\n' "$MFX_MANUAL_HOST_PID"
printf 'settings_url=%s\n' "$MFX_MANUAL_SETTINGS_URL"
printf 'log_file=%s\n' "$MFX_MANUAL_LOG_FILE"

_mfx_core_http_automation_assert_scope_match "$MFX_MANUAL_BASE_URL" "$MFX_MANUAL_SETTINGS_TOKEN" \
    "code" "process:code.exe" "true" "$tmp_dir/scope-code-vs-exe.out" "app-scope code<->exe"
_mfx_core_http_automation_assert_scope_match "$MFX_MANUAL_BASE_URL" "$MFX_MANUAL_SETTINGS_TOKEN" \
    "code.app" "process:code" "true" "$tmp_dir/scope-app-vs-base.out" "app-scope app<->base"
_mfx_core_http_automation_assert_scope_match "$MFX_MANUAL_BASE_URL" "$MFX_MANUAL_SETTINGS_TOKEN" \
    "code.exe" "process:code.app" "true" "$tmp_dir/scope-exe-vs-app.out" "app-scope exe<->app"
_mfx_core_http_automation_assert_scope_match "$MFX_MANUAL_BASE_URL" "$MFX_MANUAL_SETTINGS_TOKEN" \
    "safari" "process:code" "false" "$tmp_dir/scope-negative.out" "app-scope negative"

mfx_ok "macos automation app-scope selfcheck passed"

if [[ "$keep_running" -eq 1 ]]; then
    if [[ "$auto_stop_seconds" -gt 0 ]]; then
        mfx_manual_schedule_auto_stop "$MFX_MANUAL_HOST_PID" "$auto_stop_seconds"
        printf 'auto_stop_seconds=%s\n' "$auto_stop_seconds"
    fi
    mfx_manual_print_stop_command "$MFX_MANUAL_HOST_PID"
fi
