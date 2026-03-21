# Agent Current Context (P1, 2026-03-21)

## Purpose
- This file is the compact execution truth for daily work.
- Keep only active contracts, current boundaries, and route pointers.
- Move detailed implementation notes to targeted P2 docs.

## Scope And Priority
- Primary host: macOS.
- Delivery order: macOS first, Windows regression-free, Linux compile/contract-level.
- macOS stack rule: new capability modules are Swift-first; avoid expanding `.mm` for new large modules.
- Windows VS2026 project sync (2026-03-21) is healthy again at direct project-build level; treat any remaining `MFCMouseEffect.slnx` `ValidateSolutionConfiguration` failure as solution-metadata work, not a C++ compile regression.

## Runtime Lanes
- Stable lane: `scaffold`.
- Progressive lane: `core` (`mfx_entry_posix_host`).
- Policy: new cross-platform capability lands in `core` first, then backports as needed.

## Active Product Goals
- Keep wasm runtime bounded-but-expressive with host-owned render boundaries.
- Keep plugin lanes decoupled (`effects` vs `indicator`) with explicit diagnostics.
- Keep automation mapping accurate and observable with low regression risk.
- Rebuild `mouse_companion` on a plugin-first route with click-first visible parity.

## Capability Snapshot

### Visual Effects / WASM
- `click / trail / scroll / hold / hover` are active in `core`.
- Shared command tail (`blend_mode / sort_key / group_id`) is active.
- Group-retained model is active; transform/material/pass remain host-owned.
- Windows blacklist routing root fix is active:
  - pointer-driven effect suppression now resolves the process at the current screen point first on Windows
  - trail synthetic-follow generation is limited to a short post-input smoothing window
- Cross-platform click ripple baseline is active:
  - Windows no longer ignores `EffectConfig.ripple`
  - default click contract is now shorter, smaller, center-clear, softer-glow, and single-ring
  - double-ring regression was removed again on both Windows and macOS

### Input Indicator
- macOS/Windows label and streak semantics are aligned (`L xN`, `W+ xN`).
- Indicator wasm dispatch has dedicated lanes and a budget floor.
- `/api/wasm/load-manifest` auto-infers surface and avoids cross-surface misload.
- `SetInputIndicatorConfig` syncs runtime host immediately after apply.
- Missing/stale indicator manifest falls back cleanly to native mode.

### Plugin Management / WebUI
- Unified top-level `Plugin Management` section is active.
- WebUI apply flow is backend-state-driven (`post-apply reconcile + refresh`).
- Sidebar order is fixed:
  - `General -> Mouse Companion -> Cursor Effects -> Input Indicator -> Automation Mapping -> Plugin Management`
- Shell top-bar layout regression fix is active:
  - stable three-column layout (`brand / center status / actions`)
  - center status expands from the title boundary
  - short status stays visually one-line aligned with actions
  - long status wraps without deforming the action row

### Mouse Companion
- Backend reset remains in effect; old skeleton runtime stays removed.
- Plugin-first landing route is active (`Phase0 -> Phase1 -> Phase2`).
- Shared `IPetVisualHost` + `PlatformPetVisualHost` abstraction is the stable cross-platform visual-host seam.

#### Windows Pet
- Windows pet is in `Phase1.5`: real transparent layered host window + visible placeholder backend are active.
- Shared placement contract is active on Windows:
  - `relative`
  - `absolute`
  - legacy `fixed_bottom_left`
  - `strict / soft / free`
  - target-monitor resolution
- Current Windows placeholder is no longer just an embedded renderer; the active backend path is now:
  - `window -> backend factory/registry -> renderer input -> renderer runtime -> scene builder -> painter`
- Current Windows placeholder already consumes:
  - shared action semantics
  - `PetPoseFrame`
  - appearance/action-library asset lanes
  - lightweight readiness diagnostics
- Windows pet backend selection diagnostics are now active too:
  - `preferred_renderer_backend`
  - `selected_renderer_backend`
  - `renderer_backend_selection_reason`
  - `renderer_backend_failure_reason`
  - `available_renderer_backends`
- Current boundary:
  - visible backend is stable enough for `Phase1.5` structural work
  - Windows still does not render the real 3D model yet
  - real-renderer work should plug in behind the existing backend/runtime seams instead of reopening host/controller layers
- Read on demand:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/windows-mouse-companion-phase1-plan.md`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/windows-mouse-companion-phase15-exit-contract.md`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/windows-mouse-companion-real-renderer-contract.md`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/ops/windows-mouse-companion-manual-checklist.md`

#### macOS Pet
- macOS Phase1 visual host is active:
  - model-first with placeholder fallback
  - `.usdz` preferred when SceneKit can load it
  - runtime action updates are forwarded (`idle / follow / click_react / drag / hold_react / scroll_react`)
- Shared placement contract is active on macOS:
  - `relative`
  - `absolute`
  - legacy aliases kept for compatibility
  - `strict / soft / free`
- Runtime `size_px` resize path is active and no longer create-time-only.
- Click/head-tint parity work is active:
  - click one-shot restarts immediately
  - tint fades continuously on frame ticks
  - click stays on a tauri-style press/rebound envelope
- Idle/follow parity direction is active:
  - `hover -> idle`
  - `follow` is defined as an upright walk, not crawl
  - drag-only yaw remains separate from idle/follow facing
- Scroll direction visual is active:
  - one scroll direction keeps the pet upright
  - the opposite direction flips presentation head-down without changing skeleton solving
- Current known boundary:
  - `.usdz` framing still needs care because projected bounds can under-report height on some paths
- Read on demand:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/mouse-companion-plugin-landing-roadmap.zh-CN.md`
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/mouse-companion-action-clip-contract.zh-CN.md`

### Automation Mapping
- App-scope normalization/parser contracts are stable.
- Preset/custom gesture mapping with thresholding and ambiguity rejection is active.
- Trigger button supports `none`.
- `Draw -> Save` custom gesture flow is active.
- macOS shortcut capture/injection punctuation path is aligned.

## Observability And Debug Contract
- Runtime diagnostics are gated by debug mode where required.
- Default non-debug run avoids high-volume debug lanes.
- WebUI debug polling is adaptive and focus-aware.
- Mouse companion test route remains gated behind `MFX_ENABLE_MOUSE_COMPANION_TEST_API=1`.

## Regression Gates
- Canonical regression entry:
  - `./tools/platform/regression/run-posix-regression-suite.sh --platform auto`
- macOS daily shortcut:
  - `./mfx run-no-build` / `./mfx fast` skip both core and WebUI rebuilds
  - `./mfx run` / `./mfx start` perform fresh build preparation

## Packaging / Startup Truth

### Tray Menus
- macOS tray menu intentionally exposes only:
  - `Star Project`
  - `Settings`
  - `Exit`
- Windows tray menu now follows the same product rule.

### Launch At Startup
- macOS:
  - LaunchAgent now uses `tray` mode, not `background`
  - explicit settings toggle rewrites plist and applies `launchctl`
  - normal app startup repairs plist only and does not bootstrap/bootout service state
- Windows:
  - uses `HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run`
  - current executable path is rewritten idempotently so relocation can self-heal
  - detail doc:
    - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/windows-launch-at-startup-contract.md`

### macOS Packaging
- Preferred entrypoint:
  - `./mfx package`
- Aliases:
  - `./mfx package-no-build`
  - `./mfx pack`
  - `./mfx pkg`
- `run/start/package` now share the same macOS core/WebUI preparation helper.
- Packaged output truth:
  - standard `MFCMouseEffect.app`
  - `Install/macos`
  - folder + `.zip` + unsigned `.dmg`
- Current package policy:
  - only minimal pet runtime assets are bundled
  - wasm demo plugin keeps only runtime files
  - packaged host binary is stripped in-bundle
  - package icon is low-weight generated `MFX`
  - DMG layout is install-oriented (`left app / right Applications`)
  - `Install/macos/` is git-ignored
  - Gatekeeper/notarization is still deferred
- Detail doc:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/ops/macos-portable-packaging.md`

### Windows Packaging
- User-facing packaging entry is also `./mfx package`.
- Windows installer remains Inno Setup based under the hood.
- `mfx.cmd` is the Windows wrapper for the shared `mfx` bash entry.
- Current policy:
  - no bundled `d3dcompiler_47.dll`
  - no installer-side startup task duplication
  - install directory stays runtime-focused

## Contracts That Must Not Drift
- Keep stdin JSON command compatibility.
- Keep current wasm ABI compatibility unless migration is explicitly approved.
- Keep host-owned bounded rendering strategy; no raw shader ownership without architecture approval.
- Keep docs synchronized in the same change set for every behavior or contract change.

## P2 Routing
- P2 index:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/agent-context/p2-capability-index.md`
- Mouse companion plugin roadmap:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/mouse-companion-plugin-landing-roadmap.zh-CN.md`
- Server structure:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/server-structure.md`
- Regression workflow:
  - `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/posix-regression-suite-workflow.md`
