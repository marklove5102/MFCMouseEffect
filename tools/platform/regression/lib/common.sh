#!/usr/bin/env bash

set -euo pipefail

mfx_log() {
    local level="$1"
    shift
    printf '[mfx:%s] %s\n' "$level" "$*"
}

mfx_info() {
    mfx_log "info" "$@"
}

mfx_ok() {
    mfx_log "ok" "$@"
}

mfx_fail() {
    mfx_log "fail" "$@"
    exit 1
}

mfx_require_cmd() {
    local cmd="$1"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        mfx_fail "missing required command: $cmd"
    fi
}

mfx_assert_eq() {
    local actual="$1"
    local expected="$2"
    local context="$3"
    if [[ "$actual" != "$expected" ]]; then
        mfx_fail "$context: expected $expected, got $actual"
    fi
}

mfx_assert_file_contains() {
    local file_path="$1"
    local pattern="$2"
    local context="$3"
    if ! rg -q --fixed-strings "$pattern" "$file_path"; then
        mfx_fail "$context: missing pattern '$pattern' in $file_path"
    fi
}

mfx_http_code() {
    local output_file="$1"
    local url="$2"
    shift 2
    curl -sS -o "$output_file" -w '%{http_code}' "$@" "$url"
}
