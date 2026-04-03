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
  tools/platform/build/build-macos-project.sh [options]

Options:
  --build-dir <path>      Build directory for mfx_entry_posix_host (default: build-macos)
  --jobs <num>            Build jobs (sets MFX_BUILD_JOBS)
  --skip-webui-build      Skip rebuilding WebUIWorkspace assets
  -h, --help              Show this help
EOF
}

build_dir="$repo_root/build-macos"
skip_webui_build=0
build_jobs=""

while [[ $# -gt 0 ]]; do
    case "$1" in
    --build-dir)
        mfx_require_option_value "$1" "${2:-}"
        build_dir="$2"
        shift 2
        ;;
    --jobs)
        mfx_require_option_value "$1" "${2:-}"
        build_jobs="$2"
        shift 2
        ;;
    --skip-webui-build)
        skip_webui_build=1
        shift
        ;;
    -h|--help)
        usage
        exit 0
        ;;
    *)
        mfx_fail "unknown option for macOS build: $1"
        ;;
    esac
done

if [[ "$OSTYPE" != darwin* ]]; then
    mfx_fail "this build script is macOS-only"
fi

mfx_manual_apply_build_jobs_env "$build_jobs" "--jobs"
mfx_manual_prepare_core_host_binary "$repo_root" "$build_dir" 0 "$skip_webui_build"

printf 'build_dir=%s\n' "$build_dir"
printf 'host_bin=%s\n' "$MFX_MANUAL_HOST_BIN"
mfx_ok "macOS build ready"
