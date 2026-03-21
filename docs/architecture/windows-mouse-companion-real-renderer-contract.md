# Windows Mouse Companion Real Renderer Contract

## Purpose
- Define the interface boundary for the future Windows `Mouse Companion` real renderer.
- Ensure the current placeholder path remains a replaceable renderer backend instead of becoming a permanent special case.
- Keep renderer choice open while locking the data contract early.

## Problem Classification
- `Design behavior / future renderer interface contract`
- Short evidence:
  - Windows Phase1.5 already consumes shared inputs such as `model`, `action_library`, `appearance_profile`, `PetPoseFrame`, and runtime action state.
  - What is still missing is a renderer-facing contract that states how a future richer Windows backend plugs in without rewriting `AppController` or replacing the host lifecycle again.

## Non-Goal
This contract does **not** choose the final Windows rendering technology yet.
Examples intentionally left open:
- Direct2D
- DirectComposition
- Direct3D
- sprite-based hybrid renderer
- CPU model preview path

The goal is interface stability first, backend choice later.

## Renderer Position In The Stack
Expected long-term stack:
- shared controller/runtime
- platform visual host
- presenter/runtime coordinator
- renderer backend

Conceptually:
- `asset coordinator -> presenter/runtime -> renderer backend`

## Stable Upstream Inputs
A future Windows real renderer must be able to consume the same conceptual inputs already carried by the current host contract.

### 1. Model Lane
Input meaning:
- resolved model asset path
- resolved source format
- load success/failure diagnostics

Current source examples:
- `pet-main.usdz`
- future Windows-native converted/runtime-ready asset forms

Contract rule:
- renderer backend may choose its own runtime representation, but upstream lanes must still describe one canonical resolved model asset input

### 2. Action-Library Lane
Input meaning:
- action library path
- action clip selection state
- sampled action runtime state

Contract rule:
- action selection/sampling should remain outside the renderer where possible
- renderer should consume either sampled pose/output or compact clip-selection/runtime state, not own controller semantics

### 3. Appearance-Profile Lane
Input meaning:
- appearance profile path
- resolved color/material/accessory semantics
- load success/failure diagnostics

Contract rule:
- appearance profile remains a first-class asset lane independent of full model reload
- renderer consumes resolved appearance intent, not controller-specific branching

### 4. Pose-Frame Lane
Input meaning:
- latest `PetPoseFrame`
- per-bone semantic transforms / pose sample data
- optional pose binding state

Contract rule:
- pose transport stays shared and platform-neutral
- renderer may bind it to real bones, sprites, rig handles, or future adapters, but the upstream `PetPoseFrame` lane must not fork into a Windows-only schema

### 5. Runtime Action Lane
Input meaning:
- current semantic action (`idle / follow / click_react / drag / hold_react / scroll_react`)
- direction/facing hints
- action intensity / transient pulse state
- placement/follow anchors already normalized by presenter/runtime layers

Contract rule:
- renderer consumes action state as presentation intent
- controller and host keep ownership of action semantics and placement policy

## Required Downstream Responsibilities
A future real renderer backend is expected to own only renderer-local concerns:
- visual resource creation/destruction
- scene/mesh/sprite/model instance ownership
- per-frame draw/update application
- renderer-local material/shader/scene graph details
- renderer-local animation interpolation if needed

It should **not** own:
- top-level placement policy
- monitor/clamp rules
- controller config parsing
- launch/show/hide lifecycle policy
- cross-platform runtime semantics

## Recommended Internal Interface Split
The future Windows real renderer can be implemented behind three narrow concepts.

### Asset Loader / Resource Adapter
Responsibility:
- translate resolved model/action/appearance inputs into backend resources
- cache/reload backend resources
- report load failures back to host/runtime diagnostics

### Scene Runtime Adapter
Responsibility:
- receive runtime action + pose + facing inputs
- map them onto backend scene state
- keep renderer-frame state independent from host lifecycle glue

### Renderer Backend
Responsibility:
- perform final visual update/draw
- manage backend-specific device/context/swapchain/composition details

## Compatibility Rule With Placeholder
The current placeholder path should remain a valid backend implementation of the same high-level contract.

This means:
- placeholder and future real renderer should both be able to sit behind the same host/presenter/runtime flow
- switching renderer backend should not require changing `AppController` semantics
- switching renderer backend should not require redefining model/action/appearance/pose inputs

## Exit Criteria For Starting Real Renderer Work
Real renderer implementation work should begin only when all of the following are true:
1. Phase1.5 placeholder has been declared stable enough by `/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/architecture/windows-mouse-companion-phase15-exit-contract.md`
2. shared input lanes are already observable through diagnostics
3. presenter/runtime layers are no longer growing because of placeholder-specific draw concerns
4. there is a clear decision whether the next milestone targets:
   - higher-quality 2D replacement, or
   - true model-driven renderer parity

## First Implementation Recommendation
Before touching GPU-specific code, the safest first step is:
1. add a renderer-facing adapter interface under `Platform/windows/Pet`
2. make the current placeholder renderer conform to that interface
3. prove renderer swap does not change host/controller contracts
4. only then add a second backend implementation

## Current Progress
- The first adapter seam is now active:
  - `IWin32MouseCompanionRendererBackend`
  - `Win32MouseCompanionRendererBackendFactory`
- Built-in backend routing no longer ends at one hardcoded constructor:
  - `Win32MouseCompanionRendererBackendRegistry`
  - placeholder backend explicit registration during default-factory setup
- `Win32MouseCompanionWindow` now depends on the renderer interface/factory seam instead of directly owning `Win32MouseCompanionPlaceholderRenderer` as a concrete-only dependency.
- A renderer-facing input seam is now active too:
  - `Win32MouseCompanionRendererInput`
  - `Win32MouseCompanionRendererInputBuilder`
- The current placeholder motion/posture/action-profile/scene path now trends toward renderer-input consumption instead of reading the whole host visual state directly.
- The placeholder backend has also split its pure paint path out:
  - `Win32MouseCompanionPlaceholderRenderer` is now the backend coordinator
  - `Win32MouseCompanionPlaceholderSceneBuilder` owns placeholder scene assembly
  - `Win32MouseCompanionPlaceholderPainter` owns GDI+ drawing only
- Runtime interpretation is now moving behind a reusable adapter too:
  - `Win32MouseCompanionRendererRuntime`
  - placeholder helper layers now consume a normalized runtime view instead of repeatedly decoding raw renderer input
- Current limitation remains explicit:
  - the placeholder renderer is still the only backend implementation
  - built-in backend selection now routes through an explicit backend registry + registration step, so future Windows backends no longer need another factory/window rewrite
  - presenter/runtime/host behavior is unchanged
  - this step is an architectural handoff seam, not a visual-capability upgrade yet

## Regression Rule
Any future Windows real-renderer landing must preserve:
- shared `IPetVisualHost` usage from `AppController`
- current placement/clamp semantics
- current diagnostics field meanings
- current asset-lane separation (`model / action_library / appearance_profile`)
- current runtime semantic action labels
