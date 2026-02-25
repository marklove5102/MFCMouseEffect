# Phase 55v: WebUI WASM Load-Failure Diagnostics Surface

## Issue Classification
- Verdict: `Process debt`.
- Problem: backend now emits `last_load_failure_stage/code`, but the Svelte WASM diagnostics panel did not surface these fields, creating observability mismatch between API and UI.

## Goal
1. Surface WASM load-failure stage/code in shared Svelte settings UI.
2. Keep warning highlighting aligned with new diagnostics semantics.
3. Maintain cross-platform WebUI reuse with i18n keys for both English and Chinese.

## Implementation
- Updated diagnostics normalization and warning heuristic:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/WebUIWorkspace/src/wasm/diagnostics-model.js`
  - added:
    - `last_load_failure_stage`
    - `last_load_failure_code`
  - warning state now includes non-empty load-failure stage/code.
- Updated WASM state adapters:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/WebUIWorkspace/src/entries/wasm-main.js`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/WebUIWorkspace/src/wasm/WasmPluginFields.svelte`
  - both adapters now carry new fields from normalized diagnostics.
- Updated diagnostics panel rendering:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/WebUIWorkspace/src/wasm/WasmPluginFields.svelte`
  - added rows:
    - `Last load failure stage`
    - `Last load failure code`
- Updated i18n keys:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/WebUI/i18n.js`
  - English + Chinese keys:
    - `label_wasm_last_load_failure_stage`
    - `label_wasm_last_load_failure_code`
- Rebuilt WebUI workspace output:
  - `pnpm --dir MFCMouseEffect/WebUIWorkspace run build`

## Validation
- `pnpm --dir MFCMouseEffect/WebUIWorkspace run build`
- `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- `./tools/docs/doc-hygiene-check.sh --strict`

## Closure
- WASM load-failure diagnostics are now visible in the shared Svelte settings UI, and warning highlighting reflects both budget/parse issues and explicit load-failure stage/code signals.
