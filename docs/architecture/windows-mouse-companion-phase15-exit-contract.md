# Windows Mouse Companion Phase1.5 Exit Contract

## Purpose
- Define what it means for the current Windows `Mouse Companion` placeholder path to be considered stable enough to stop expanding and start serving as a handoff layer.
- Prevent the 2D placeholder lane from growing into a permanent mixed-responsibility renderer.
- Provide a concrete exit gate before the project invests in a richer Windows renderer backend.

## Problem Classification
- `Design behavior / phase-boundary contract`
- Short evidence:
  - Windows `Mouse Companion` already has a transparent layered host, shared placement contract, runtime pose/action/appearance consumption, and a dedicated placeholder presenter/renderer split.
  - The repository currently has a detailed implementation-status narrative and a manual checklist, but it does not yet have a compact definition of "Phase1.5 done" versus "still in active expansion".

## Phase1.5 Role
- Windows Phase1.5 is a **stable visible placeholder** phase.
- It is not the final visual parity target.
- It exists to prove:
  - shared runtime contracts are sufficient
  - Windows host/presenter/runtime boundaries are sound
  - a future real renderer can replace only the renderer/backend layer instead of forcing another controller rewrite

## Exit Decision
Windows `Mouse Companion` Phase1.5 may be considered complete only when all of the following are true:
1. The placeholder path is behaviorally distinguishable and manually verifiable.
2. The shared config/runtime asset lanes are stable and observable.
3. The renderer/presenter/runtime boundaries are no longer expanding for every visual tweak.
4. Remaining gaps are explicitly in the "real renderer" category rather than basic host/runtime correctness.

## Must-Pass Gates

### 1. Host Lifecycle Gate
The following user-visible behaviors must be stable:
- enable -> visible pet host appears once
- disable -> host hides immediately
- repeated apply/reload does not leave stale ghost windows
- shutdown exits cleanly without leaving the placeholder window behind

### 2. Placement Gate
The Windows host must correctly consume the shared placement contract:
- `relative`
- `absolute`
- legacy `fixed_bottom_left`
- `strict / soft / free`
- `target_monitor`

Expected stable behavior:
- first-show `relative` fallback resolves against selected monitor center before the first pointer sample
- monitor routing is deterministic
- clamp mode changes are visibly distinct and do not require renderer rewrites

### 3. Action-Semantics Gate
The placeholder must keep these states visibly distinct:
- `idle`
- `follow`
- `click_react`
- `drag`
- `hold_react`
- `scroll_react`

Expected stable behavior:
- the user can tell these states apart without diagnostics
- `scroll_react` remains directional
- `follow/drag` preserve readable facing tendency without tiny-motion jitter
- reaction pulses decay instead of latching forever

### 4. Runtime-Consumption Gate
The placeholder must continue to consume shared runtime lanes, not only action names:
- `PetPoseFrame`
- sampled action-library output
- appearance-profile data
- host placement/config snapshot

Expected stable behavior:
- removing the real-model lane does not break placeholder runtime semantics
- the placeholder remains a meaningful consumer of future renderer input, not a separate bespoke contract

### 5. Diagnostics Gate
At least these runtime fields must remain trustworthy:
- `visual_host_active`
- `model_loaded`
- `action_library_loaded`
- `appearance_profile_loaded`
- `loaded_model_path`
- `loaded_action_library_path`
- `loaded_appearance_profile_path`
- `model_load_error`
- `action_library_load_error`
- `appearance_profile_load_error`

Rule:
- diagnostics must describe real host/runtime state, not optimistic intent only

## Acceptable Remaining Boundaries
The following may remain unfinished at Phase1.5 exit and are **not** regressions by themselves:
- `model_loaded` can still be `false` on Windows
- placeholder remains non-GPU
- no real 3D model playback yet
- no final bone-driven renderer parity with macOS
- no renderer-backend commitment yet (`D2D/DComp/D3D/...`)

These are allowed only if the placeholder path is otherwise stable and the future renderer can consume the same asset/runtime contract.

## Not Allowed At Exit
Phase1.5 must **not** exit with these unresolved categories:
- host lifecycle bugs
- placement/clamp regressions
- basic action states collapsing into one unreadable silhouette
- config apply requiring controller-side platform branching growth
- renderer changes that still require `AppController` semantic rewrites
- new placeholder tweaks that repeatedly force changes across host/runtime/renderer in one patch

## Structural Exit Rule
Before leaving Phase1.5, responsibilities should read roughly like this:
- `AppController`
  - shared runtime/config ownership
  - path resolution
  - diagnostics ownership
- `PetVisualAssetCoordinator`
  - asset-lane apply normalization
- `Win32MouseCompanionVisualHost`
  - lifecycle coordination only
- `Win32MouseCompanionPresenter`
  - placement/clamp/display policy
- `Win32MouseCompanionVisualRuntime`
  - action/facing/pose/application state normalization
- `Win32MouseCompanionActionRuntime`
  - action clip selection/sampling
- placeholder helpers
  - build draw-ready state from runtime inputs
- renderer
  - paint only, or as close as practical

Exit rule:
- if a new placeholder feature still has to punch through all layers at once, Phase1.5 is not yet really closed.

## Manual Verification Requirement
Phase1.5 exit requires at minimum one full pass of:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/ops/windows-mouse-companion-manual-checklist.md`

Minimum mandatory sections:
- `Build Gate`
- `Smoke Path`
- `Placement`
- `Action Semantics`
- `Runtime Diagnostics`

## Entry Condition For Next Phase
The project may move from Phase1.5 closure to real-renderer preparation only when:
1. the placeholder path is intentionally declared stable
2. unresolved work is mostly renderer-backend capability, not host/runtime architecture debt
3. renderer input lanes are explicitly documented in the real-renderer contract

## Recommended Next Step After Exit
Once this contract is satisfied, the next document to read/maintain is:
- `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/windows-mouse-companion-real-renderer-contract.md`
