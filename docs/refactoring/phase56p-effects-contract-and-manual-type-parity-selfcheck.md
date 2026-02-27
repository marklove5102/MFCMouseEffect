# Phase 56p - Effects Contract and Manual Type-Parity Selfcheck

## Why
- Effects parity closure needs repeatable verification beyond visual spot checks.
- Existing effects contract checks already covered profile/overlay basics, but needed stronger guards for:
  - active normalized sample visibility
  - additional alias mappings (`click none`, `trail default`, `trail scifi`, `scroll none`, `hover none`)
- Manual mac verification also needed a one-command flow focused on five-category type parity.

## What Changed
1. Expanded effects contract assertions:
- `tools/platform/regression/lib/core_http_automation_contract_effect_overlay_checks.sh`
- Added checks for:
  - `sample_input.active_raw`
  - `sample_input.active_normalized`
  - additional alias matrix mappings listed above.

2. Added macOS manual type-parity selfcheck script:
- `tools/platform/manual/run-macos-effects-type-parity-selfcheck.sh`
- Coverage:
  - probes alias matrix and normalized samples
  - applies five-category active types via `/api/state`
  - verifies persisted active types + hold follow-mode alias normalization (`cursor_priority -> smooth`)
  - runs overlay probe for all five categories with explicit types

## Validation
```bash
./tools/platform/manual/run-macos-effects-type-parity-selfcheck.sh
./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto
```
