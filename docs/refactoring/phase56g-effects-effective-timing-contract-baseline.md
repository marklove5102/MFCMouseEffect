# Phase 56g - Effects Effective Timing Contract Baseline

## Goal
Provide a stable, machine-checkable timing baseline for mac effect command outputs, so Windows-vs-mac behavior alignment can be measured by command-level timing fields before pixel-level rendering tuning.

## Change
- Extended `/api/effects/test-render-profiles` `command_samples` with:
  - `effective_timing.click_duration_sec`
  - `effective_timing.trail_duration_sec`
  - `effective_timing.scroll_duration_sec`
  - `effective_timing.hover_breathe_duration_sec`
  - `effective_timing.hover_spin_duration_sec`
  - `effective_timing.hold_progress_full_ms`
  - `effective_timing.hold_breathe_duration_sec`
- Added regression assertions for the new timing baseline section/fields.

## Files
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.EffectsProfileStateBuilder.Macos.cpp`
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_effect_overlay_checks.sh`

## Validation
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
  - passed

## Risk
- Low: diagnostics/test payload extension only.
