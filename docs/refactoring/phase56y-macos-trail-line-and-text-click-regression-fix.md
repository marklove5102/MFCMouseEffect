# Phase 56y - macOS Trail Line + Click Text Regression Fix

## Why
- User-visible regressions were reported on macOS:
  - `trail=line` rendered detached short bars ("matchsticks").
  - `click=text` had no visible output in runtime tests.

## Decision
- Classification: `Bug/regression`.
- Keep Windows behavior unchanged.
- Fix macOS runtime path directly instead of adding probe-only patches.

## Changes
1. Trail line emission
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseEffect.mm`
- Removed `line` segmented-emission branch (`ShouldSegmentEmission` + interpolation loop).
- Restored single-command continuous emission path:
  - one `ComputeTrailEffectRenderCommand(...)`
  - one `ShowTrailPulseOverlay(...)` per accepted move emission.

2. Trail line path shaping
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTrailPulseOverlayStyle.mm`
- For `trail=line`, removed short fixed-segment clamp (`14..28px`) and switched to real delta-length path.
- Keeps non-line trail type clamps (`meteor/electric/...`) unchanged.

3. Line trail throttle gap reduction
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosEffectRenderProfile.Shared.h`
- Tightened line throttle profile from `8ms/3px` to `5ms/1.5px` to reduce dotted-matchstick look.

4. Click text runtime robustness
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Effects/TextEffect.cpp`
- Added default text fallback when text pool is empty:
  - `LEFT` / `RIGHT` / `MIDDLE` by mouse button.
- On macOS, text click now prioritizes platform fallback rendering directly:
  - `MacosTextEffectFallback` is attempted first for non-emoji text.
- Keeps existing cross-platform fallback chain for non-mac hosts.

5. Click type normalization hardening
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Effects/ClickEffectCompute.cpp`
- Added `TrimAscii` preprocessing and broader text alias matching (`contains("text")`).
- Prevents UI-side variant strings from accidentally falling back to non-text click type.

6. Text settings hot-reapply guard
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/MouseFx/Core/Control/AppController.ConfigUpdates.cpp`
- Reapply condition switched from raw-string check to normalized check:
  - `NormalizeClickEffectType(config_.active.click) == "text"`.
- Prevents alias forms from silently missing runtime refresh.

7. Global edge anchor behavior fix
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosOverlayRenderSupport.mm`
- Removed frame-origin clamp-to-screen behavior in `ClampOverlayFrameToScreenBounds`.
- Policy changed to:
  - keep real input anchor position
  - allow partial off-screen clipping near edges
  - avoid visible inward drift of effects at screen boundaries

8. macOS text size parity alignment
- File: `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/MFCMouseEffect/Platform/macos/Effects/MacosTextEffectFallback.mm`
- Converted text size from point semantics to px parity baseline (`pt * 96 / 72`) with minimum floor.
- Improves visibility consistency with Windows text effect baseline.

## Validation
1. Build
- `cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8`

2. Effects contracts
- `./tools/platform/regression/run-posix-core-effects-contract-regression.sh --platform auto`

3. Type parity selfcheck
- `./tools/platform/manual/run-macos-effects-type-parity-selfcheck.sh --build-dir /tmp/mfx-platform-macos-core-build --skip-build`

## Risk
- Click text path on macOS now explicitly prefers fallback renderer; visual timing may differ slightly from previous overlay-host fallback chain.
- Windows path is untouched.
