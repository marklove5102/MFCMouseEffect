# Phase 56m - Hold Follow-Mode Alias Unification

## Why
- `hold_follow_mode` normalization drift existed across paths:
  - core apply path used local fallback normalization
  - hold compute parser had separate mode parsing
  - scaffold state patch only accepted strict canonical values
- This could cause inconsistent behavior when clients send semantic aliases like `cursor_priority` or `performance_first`.

## What Changed
1. Added shared normalization contract in config internals:
- `TryNormalizeHoldFollowMode(const std::string&, std::string*)`
- `NormalizeHoldFollowMode(std::string)`

2. Unified all call sites to the shared contract:
- core runtime apply path (`AppController::SetHoldFollowMode`)
- hold compute parser (`ParseHoldEffectFollowMode`)
- scaffold settings patch validation + persistence

3. Added follow-mode alias matrix visibility to effects probe:
- `/api/effects/test-render-profiles` `command_samples.alias_matrix.hold_follow_mode`

4. Extended regression assertions:
- effects contract now verifies:
  - `cursor_priority -> smooth`
  - `performance_first -> efficient`
  - `cpu_saver -> efficient`

## Alias Canonicalization Contract
- `precise`: `precise`, `low_latency`, `latency_first`, `raw`
- `smooth`: `smooth`, `cursor_priority`, `cursor_first`, `recommended`
- `efficient`: `efficient`, `performance_first`, `cpu_saver`, `powersave`, `power_save`
- separator normalization: `-`/space -> `_`, and case-insensitive.

## Risks
- Behavior is intentionally broadened (more accepted aliases), but canonical persisted values remain unchanged (`precise|smooth|efficient`).
- Unknown values remain rejected by scaffold patch and default to `smooth` in runtime normalization fallback, matching prior safety behavior.

## Validation
```bash
./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto
```
