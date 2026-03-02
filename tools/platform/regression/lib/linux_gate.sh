#!/usr/bin/env bash

set -euo pipefail

mfx_run_linux_compile_gate() {
    local repo_root="$1"
    local build_dir="$2"

    local targets=("mfx_shell_linux" "mfx_entry_posix")

    mfx_info "configure linux package (cross-host enabled)"
    cmake -S "$repo_root/MFCMouseEffect/Platform" -B "$build_dir" \
        -DMFX_PACKAGE_PLATFORM=linux \
        -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON \
        -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON

    mfx_info "build targets: ${targets[*]}"
    cmake --build "$build_dir" --target "${targets[@]}" -j"${MFX_BUILD_JOBS:-8}"
    mfx_ok "linux compile gate passed"
}
