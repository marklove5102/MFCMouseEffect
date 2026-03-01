# Phase 56zzn - macOS Trail Planner Diagnostics Surfacing

## Scope
- Capability: `effects` (trail diagnostics/contracts).
- Goal:
  - surface trail emission planner runtime values into state/profile APIs.
  - make planner tuning verifiable without manual source inspection.

## Decision
- Expose planner values in both:
  - `/api/state` -> `effects_profile.trail_emission_planner`
  - `/api/effects/test-render-profiles` -> `command_samples.effective_timing`
- Contract script must assert these keys to prevent silent removal.

## Code Changes
1. State/profile mapper:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.EffectsProfileStateBuilder.Macos.cpp`
  - added planner include.
  - reads `ResolveTrailPulseEmissionPlannerConfig()`.
  - emits:
    - `trail_emission_planner.teleport_skip_distance_px`
    - `trail_emission_planner.max_segments`
  - extends effective timing sample with:
    - `trail_planner_teleport_skip_distance_px`
    - `trail_planner_max_segments`

2. Contract gate:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_effect_overlay_checks.sh`
  - asserts planner fields exist in render-profile probe and state output.

## Verification
- `cmake --build /tmp/mfx-platform-macos-build --target mfx_entry_posix_host -j8`
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
- `./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto`
- `./tools/platform/regression/run-macos-objcxx-surface-regression.sh`

All commands passed on macOS.

## Result
- Planner telemetry is now first-class diagnostics output.
- Regression contracts lock this visibility to avoid future drift.
