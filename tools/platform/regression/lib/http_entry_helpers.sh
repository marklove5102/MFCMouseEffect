#!/usr/bin/env bash

set -euo pipefail

_mfx_http_entry_pid=""
_mfx_http_fifo_path=""
_mfx_http_fifo_writer_pid=""

_mfx_http_start_entry() {
    local entry_bin="$1"
    local log_file="$2"
    shift 2

    _mfx_http_fifo_path="$(mktemp -u "/tmp/mfx-posix-http-fifo.XXXXXX")"
    mkfifo "$_mfx_http_fifo_path"

    tail -f /dev/null >"$_mfx_http_fifo_path" &
    _mfx_http_fifo_writer_pid="$!"

    if [[ $# -gt 0 ]]; then
        env "$@" "$entry_bin" -mode=background <"$_mfx_http_fifo_path" >"$log_file" 2>&1 &
    else
        "$entry_bin" -mode=background <"$_mfx_http_fifo_path" >"$log_file" 2>&1 &
    fi
    _mfx_http_entry_pid="$!"

    sleep "${MFX_HTTP_SERVER_WAIT_SECONDS:-1}"
    if ! kill -0 "$_mfx_http_entry_pid" >/dev/null 2>&1; then
        mfx_info "entry startup log:"
        cat "$log_file" || true
        mfx_fail "entry process exited before HTTP checks"
    fi
}

_mfx_http_stop_entry() {
    if [[ -n "$_mfx_http_fifo_path" && -p "$_mfx_http_fifo_path" ]]; then
        printf 'exit\n' >"$_mfx_http_fifo_path" || true
    fi

    if [[ -n "$_mfx_http_entry_pid" ]]; then
        wait "$_mfx_http_entry_pid" >/dev/null 2>&1 || true
    fi

    if [[ -n "$_mfx_http_fifo_writer_pid" ]]; then
        kill "$_mfx_http_fifo_writer_pid" >/dev/null 2>&1 || true
        wait "$_mfx_http_fifo_writer_pid" >/dev/null 2>&1 || true
    fi

    if [[ -n "$_mfx_http_fifo_path" ]]; then
        rm -f "$_mfx_http_fifo_path"
    fi

    _mfx_http_entry_pid=""
    _mfx_http_fifo_path=""
    _mfx_http_fifo_writer_pid=""
}
