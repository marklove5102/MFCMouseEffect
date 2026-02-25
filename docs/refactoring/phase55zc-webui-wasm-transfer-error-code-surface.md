# Phase 55zc: WebUI WASM Transfer Error-Code Surface

## Issue Classification
- Verdict: `Process debt`.
- Problem: backend now returns stable transfer `error_code`, but WebUI still primarily displayed free-form `error` text and did not expose machine-readable code to users.

## Goal
1. Surface transfer `error_code` in shared Svelte WASM settings UI.
2. Add frontend-side error-code message mapping without coupling large mapping tables into view files.
3. Keep EN/ZH UI readable and backward-compatible when `error_code` is absent.

## Implementation
- Added error-code model module:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/WebUIWorkspace/src/wasm/action-error-model.js`
  - contains:
    - `normalizeActionErrorCode`
    - `resolveWasmActionErrorMessage`
    - centralized transfer error-code to fallback message mapping.
- Updated WASM settings view:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/WebUIWorkspace/src/wasm/WasmPluginFields.svelte`
  - action failure path now consumes `response.error_code`,
  - operation result includes `Error code: <code>` when present,
  - unknown codes still show fallback generic message.
- Updated UI translations:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/WebUI/i18n.js`
  - added `label_wasm_error_code` for EN/ZH.

## Validation
- `pnpm --dir MFCMouseEffect/WebUIWorkspace run build`
- `pnpm --dir MFCMouseEffect/WebUIWorkspace run test:automation-platform`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`

## Closure
- Transfer error-code contract is now visible and actionable in WebUI, reducing backend/frontend contract drift and improving diagnosis quality for import/export failures.
