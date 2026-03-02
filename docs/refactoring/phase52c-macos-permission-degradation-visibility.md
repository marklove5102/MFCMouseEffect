# Phase 52c: macOS Permission Degradation Visibility + Capability-Driven Effects UI

## Top Decisions (Important)
- Keep Windows startup behavior unchanged (global hook failure remains fatal on Windows).
- For macOS core lane, keep process alive on hook failure but expose explicit degraded state through:
  - shell-level user notification,
  - Web settings runtime state/banner.
- Consume `schema.capabilities.effects` in WebUI to prevent selecting unsupported categories on current platform.

## Changes
1. Added structured input-capture runtime status in core controller:
   - Updated `MFCMouseEffect/MouseFx/Core/Control/AppController.h`
   - Updated `MFCMouseEffect/MouseFx/Core/Control/AppController.cpp`
   - Added `InputCaptureRuntimeStatus` + `InputCaptureFailureReason`.
   - Tracked runtime state with atomics (`active`, `error`).
   - Classified failures (`permission_denied`, `unsupported`, `start_failed`) in platform-aware way.
2. Exposed degraded permission/runtime state to Web settings:
   - Updated `MFCMouseEffect/MouseFx/Server/SettingsStateMapper.cpp`
   - Added `input_capture` object in `/api/state`.
   - Added `input_capture_notice` with localized user-facing guidance.
3. Added shell-level degraded warning on core startup:
   - Updated `MFCMouseEffect/Platform/posix/Shell/PosixCoreAppShell.cpp`
   - When AppController starts in degraded mode, shell notifier now shows a clear warning message.
4. Wired Web settings banner to prefer input-permission degradation notice:
   - Updated `MFCMouseEffect/WebUI/app.js`
   - Added unified runtime notice selection with priority:
     - `input_capture_notice`
     - fallback to `gpu_route_notice`
5. Consumed schema capability in effects UI (Svelte source first):
   - Updated `MFCMouseEffect/WebUIWorkspace/src/entries/effects-main.js`
   - Updated `MFCMouseEffect/WebUIWorkspace/src/effects/ActiveEffectsFields.svelte`
   - Added `effectCapabilities` prop and disabled unsupported effect selects.
   - Added capability-limited hint text.
6. Added i18n and legacy fallback behavior:
   - Updated `MFCMouseEffect/WebUI/i18n.js`
   - Updated `MFCMouseEffect/WebUI/settings-form.js`
   - Added `hint_effect_capability_limited` (en/zh-CN).
   - Legacy DOM fallback path also disables unsupported effect selects.

## Validation
- Rebuilt effects Svelte bundle and synced to runtime WebUI output:
  - `cd MFCMouseEffect/WebUIWorkspace`
  - `pnpm run build:effects`
  - `node scripts/copy-output.mjs`
- Full platform guardrails regression (same phase boundary checks):
  - macOS core lane compile + smoke (`mfx_entry_posix_host`)
  - scaffold regression script
  - Linux compile-level follow (`mfx_shell_linux`, `mfx_entry_posix`)

## Risk / Follow-up
- This phase provides visible degraded guidance and capability-driven disablement, but manual permission toggle acceptance cases should be explicitly recorded in next doc slice (52d).
- `input_capture_notice` currently takes status-banner priority over GPU fallback notice to keep permission remediation obvious on macOS.
