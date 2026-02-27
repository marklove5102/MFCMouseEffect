#!/usr/bin/env bash

set -euo pipefail

_mfx_posix_suite_require_value() {
    local flag="$1"
    if [[ $# -lt 2 || -z "${2:-}" ]]; then
        mfx_fail "missing value for $flag"
    fi
}

_mfx_posix_suite_normalize_toggle() {
    local raw="$1"
    case "$raw" in
        1|true|TRUE|True|yes|YES|on|ON) printf '1' ;;
        0|false|FALSE|False|no|NO|off|OFF) printf '0' ;;
        *)
            mfx_fail "invalid toggle value: $raw (expected 0/1 or true/false)"
            ;;
    esac
}

mfx_posix_suite_init_defaults() {
    MFX_PLATFORM="auto"
    MFX_SCAFFOLD_BUILD_DIR=""
    MFX_CORE_BUILD_DIR=""
    MFX_CORE_AUTOMATION_BUILD_DIR=""
    MFX_LINUX_BUILD_DIR="/tmp/mfx-platform-linux-build"
    MFX_BUILD_JOBS_VALUE=""

    MFX_SKIP_SCAFFOLD=0
    MFX_SKIP_CORE_SMOKE=0
    MFX_SKIP_CORE_AUTOMATION=0
    MFX_SKIP_MACOS_AUTOMATION_INJECTION_SELFCHECK=0
    MFX_SKIP_LINUX_GATE=0
    MFX_LINUX_SKIP_CORE_RUNTIME=0
    MFX_SKIP_AUTOMATION_TEST=0
    MFX_SKIP_MACOS_WASM_SELFCHECK=0
    MFX_SCAFFOLD_SKIP_SMOKE=0
    MFX_SCAFFOLD_SKIP_HTTP=0
    MFX_ENFORCE_NO_OBJCXX_EDITS="$(_mfx_posix_suite_normalize_toggle "${MFX_ENFORCE_NO_OBJCXX_EDITS:-0}")"
}

mfx_posix_suite_print_usage() {
    cat <<'USAGE'
Usage: run-posix-regression-suite.sh [options]
  --platform <auto|macos|linux>   scaffold package platform (default: auto)
  --scaffold-build-dir <path>     scaffold regression build dir override
  --core-build-dir <path>         core-lane smoke build dir override
  --core-automation-build-dir <path> core automation HTTP contract build dir override
  --linux-build-dir <path>        linux compile gate build dir (default: /tmp/mfx-platform-linux-build)
  --jobs <n>                      build jobs override (default: env MFX_BUILD_JOBS or 8)
  --skip-scaffold                 skip scaffold regression phase
  --skip-core-smoke               skip core-lane smoke phase
  --skip-core-automation          skip core automation HTTP contract phase
  --skip-macos-automation-injection-selfcheck skip macOS automation injection selfcheck phase
  --skip-linux-gate               skip linux compile gate phase
  --linux-skip-core-runtime       forward: linux gate skips core-runtime lane compile
  --skip-automation-test          skip webui automation platform semantic tests
  --skip-macos-wasm-selfcheck     skip macOS wasm runtime selfcheck phase
  --scaffold-skip-smoke           forward: skip scaffold smoke checks
  --scaffold-skip-http            forward: skip scaffold HTTP checks
  --enforce-no-objcxx-edits       fail when workspace edits contain .mm/.m (policy gate)
USAGE
}

mfx_posix_suite_parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --platform)
                _mfx_posix_suite_require_value "$1" "${2:-}"
                MFX_PLATFORM="$2"
                shift 2
                ;;
            --scaffold-build-dir)
                _mfx_posix_suite_require_value "$1" "${2:-}"
                MFX_SCAFFOLD_BUILD_DIR="$2"
                shift 2
                ;;
            --core-build-dir)
                _mfx_posix_suite_require_value "$1" "${2:-}"
                MFX_CORE_BUILD_DIR="$2"
                shift 2
                ;;
            --core-automation-build-dir)
                _mfx_posix_suite_require_value "$1" "${2:-}"
                MFX_CORE_AUTOMATION_BUILD_DIR="$2"
                shift 2
                ;;
            --linux-build-dir)
                _mfx_posix_suite_require_value "$1" "${2:-}"
                MFX_LINUX_BUILD_DIR="$2"
                shift 2
                ;;
            --jobs)
                _mfx_posix_suite_require_value "$1" "${2:-}"
                MFX_BUILD_JOBS_VALUE="$2"
                shift 2
                ;;
            --skip-scaffold)
                MFX_SKIP_SCAFFOLD=1
                shift
                ;;
            --skip-core-smoke)
                MFX_SKIP_CORE_SMOKE=1
                shift
                ;;
            --skip-core-automation)
                MFX_SKIP_CORE_AUTOMATION=1
                shift
                ;;
            --skip-macos-automation-injection-selfcheck)
                MFX_SKIP_MACOS_AUTOMATION_INJECTION_SELFCHECK=1
                shift
                ;;
            --skip-linux-gate)
                MFX_SKIP_LINUX_GATE=1
                shift
                ;;
            --linux-skip-core-runtime)
                MFX_LINUX_SKIP_CORE_RUNTIME=1
                shift
                ;;
            --skip-automation-test)
                MFX_SKIP_AUTOMATION_TEST=1
                shift
                ;;
            --skip-macos-wasm-selfcheck)
                MFX_SKIP_MACOS_WASM_SELFCHECK=1
                shift
                ;;
            --scaffold-skip-smoke)
                MFX_SCAFFOLD_SKIP_SMOKE=1
                shift
                ;;
            --scaffold-skip-http)
                MFX_SCAFFOLD_SKIP_HTTP=1
                shift
                ;;
            --enforce-no-objcxx-edits)
                MFX_ENFORCE_NO_OBJCXX_EDITS=1
                shift
                ;;
            -h|--help)
                mfx_posix_suite_print_usage
                exit 0
                ;;
            *)
                mfx_fail "unknown argument: $1"
                ;;
        esac
    done
}

mfx_posix_suite_export_build_jobs() {
    if [[ -z "$MFX_BUILD_JOBS_VALUE" ]]; then
        return
    fi
    if ! [[ "$MFX_BUILD_JOBS_VALUE" =~ ^[0-9]+$ ]] || [[ "$MFX_BUILD_JOBS_VALUE" -le 0 ]]; then
        mfx_fail "invalid --jobs value: $MFX_BUILD_JOBS_VALUE"
    fi
    export MFX_BUILD_JOBS="$MFX_BUILD_JOBS_VALUE"
}

mfx_posix_suite_is_macos_host() {
    [[ "$(uname -s)" == "Darwin" ]]
}
