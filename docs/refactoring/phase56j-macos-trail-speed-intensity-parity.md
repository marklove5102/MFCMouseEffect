# Phase 56j - macOS Trail Speed/Intensity Parity

## Goal
Improve macOS trail behavior parity by introducing continuous motion-strength data into shared trail commands and using it in mac renderer styling.

## Change

### 1) Shared trail command model
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Effects/TrailEffectCompute.h`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Effects/TrailEffectCompute.cpp`
- Added command fields:
  - `speed_px` (delta magnitude)
  - `intensity` (normalized motion strength)
- Compute now applies speed-informed scaling to:
  - `size_px`
  - `duration_sec`

### 2) macOS trail renderer
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Layers.mm`
- Uses command intensity to smooth visual response:
  - line/tube/particle stroke width scaling
  - core/glow opacity bias

### 3) Contract observability
- Updated:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Server/SettingsStateMapper.EffectsProfileStateBuilder.Macos.cpp`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/tools/platform/regression/lib/core_http_automation_contract_effect_overlay_checks.sh`
- `command_samples.trail` now exposes and asserts `speed_px`.

## Validation
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`
  - passed

## Risk
- Low-medium:
  - trail visual intensity mapping changed for faster/slower moves.
  - API compatibility unchanged (diagnostics fields expanded only).
