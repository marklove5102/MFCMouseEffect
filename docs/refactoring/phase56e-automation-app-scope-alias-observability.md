# phase56e: automation app-scope alias observability hardening

## Why
- `process:code` / `process:code.app` / `process:code.exe` matching was implemented, but contract visibility was weak.
- Regression checks should assert alias expansion explicitly, not only `matched=true/false`.

## Changes
- Extended automation scope test API response:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.TestAutomationScopeApiRoutes.cpp`
  - new fields:
    - `process_aliases`
    - `app_scope_alias_matrix` (`input`, `normalized`, `aliases`)
- Extended contract checks:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_app_scope_checks.sh`
  - now asserts alias arrays for `code`, `code.app`, `code.exe` samples.

## Validation
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build`
- Result: passed.
