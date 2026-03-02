#!/usr/bin/env bash

set -euo pipefail

mfx_configure_platform_build_dir() {
    local repo_root="$1"
    local build_dir="$2"
    local platform="$3"
    shift 3

    mfx_info "configure platform package: $platform"
    cmake -S "$repo_root/MFCMouseEffect/Platform" -B "$build_dir" \
        -DMFX_PACKAGE_PLATFORM="$platform" \
        -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON \
        "$@"
}

mfx_default_regression_targets() {
    local platform="$1"
    local targets=("mfx_entry_posix_host")
    if [[ "$platform" == "macos" ]]; then
        targets+=("mfx_shell_macos_smoke" "mfx_shell_macos_tray_smoke")
    elif [[ "$platform" == "linux" ]]; then
        targets+=("mfx_shell_linux")
    fi
    printf '%s\n' "${targets[@]}"
}

mfx_build_targets() {
    local build_dir="$1"
    shift
    if [[ $# -eq 0 ]]; then
        mfx_fail "build target list is empty"
    fi
    mfx_info "build targets: $*"
    cmake --build "$build_dir" --target "$@" -j"${MFX_BUILD_JOBS:-8}"
    mfx_ok "build completed"
}

mfx_configure_and_build() {
    local repo_root="$1"
    local build_dir="$2"
    local platform="$3"
    shift 3

    mfx_configure_platform_build_dir "$repo_root" "$build_dir" "$platform" "$@"

    local targets=()
    while IFS= read -r target; do
        targets+=("$target")
    done < <(mfx_default_regression_targets "$platform")
    mfx_build_targets "$build_dir" "${targets[@]}"
}

mfx_configure_and_build_entry_host() {
    local repo_root="$1"
    local build_dir="$2"
    local platform="$3"
    shift 3

    mfx_configure_platform_build_dir "$repo_root" "$build_dir" "$platform" "$@"
    mfx_build_targets "$build_dir" "mfx_entry_posix_host"
}
