# Phase 53g: AppScope Match Test Endpoint and Contract Closure

## Issue Classification
- Verdict: `Manual-acceptance debt reduction`.
- Problem: `phase53b` app-scope alias matching (`process:code.exe / process:code / process:code.app`) remained manual-only, causing repeated hand checks and drift risk.

## Goal
1. Expose a test-gated API to verify app-scope matching semantics directly in core lane.
2. Reuse shared matcher logic to avoid duplicated behavior.
3. Close `phase53b` acceptance via scripted contract checks.

## Implementation
- Shared matcher extraction:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Automation/AppScopeUtils.h`
  - added:
    - `AppScopeMatchesProcess(...)`
    - `AppScopeSpecificity(...)`
- Engine now delegates to shared matcher:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Automation/InputAutomationEngine.cpp`
- Test-gated HTTP endpoint:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/WebSettingsServer.Routing.cpp`
  - `POST /api/automation/test-app-scope-match`
  - env gate: `MFX_ENABLE_AUTOMATION_SCOPE_TEST_API=1`
  - returns normalized process/scope fields plus `matched` and `specificity`.
- Contract regression extension:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http.sh`
  - verifies:
    - `code` vs `process:code.exe` -> `matched=true`
    - `code.app` vs `process:code` -> `matched=true`
    - `code.exe` vs `process:code.app` -> `matched=true`
    - `safari` vs `process:code` -> `matched=false`
  - default-enables `MFX_ENABLE_AUTOMATION_SCOPE_TEST_API`.
- Help text update:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/run-posix-core-automation-contract-regression.sh`
- Workflow doc update:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/posix-core-automation-contract-workflow.md`

## Validation
- `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build`
- `./tools/platform/manual/run-macos-wasm-runtime-selfcheck.sh --skip-build`
