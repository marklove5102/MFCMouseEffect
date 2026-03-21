#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererInputBuilder.h"

namespace mousefx::windows {

Win32MouseCompanionRendererInput BuildWin32MouseCompanionRendererInput(
    const Win32MouseCompanionVisualState& state) {
    Win32MouseCompanionRendererInput input{};
    input.config = state.config;
    input.modelPath = state.lastModelPath;
    input.actionLibraryPath = state.lastActionLibraryPath;
    input.actionName = state.lastActionName;
    input.reactiveActionName = state.lastReactiveActionName;
    input.actionCode = state.lastActionCode;
    input.facingDirection = state.facingDirection;
    input.actionIntensity = state.lastActionIntensity;
    input.headTintAmount = state.lastHeadTintAmount;
    input.facingMomentumPx = state.facingMomentumPx;
    input.scrollSignedIntensity = state.lastScrollSignedIntensity;
    input.reactiveActionIntensity = state.lastReactiveActionIntensity;
    input.poseSampleTickMs = state.lastPoseSampleTickMs;
    input.clickTriggerTickMs = state.lastClickTriggerTickMs;
    input.holdTriggerTickMs = state.lastHoldTriggerTickMs;
    input.scrollTriggerTickMs = state.lastScrollTriggerTickMs;
    input.latestPoseFrame = state.latestPoseFrame;
    input.latestActionClipSample = state.latestActionClipSample;
    input.appearanceProfile = state.appearanceProfile;
    input.modelAssetAvailable = state.modelAssetAvailable;
    input.actionLibraryAvailable = state.actionLibraryAvailable;
    input.poseFrameAvailable = state.poseFrameAvailable;
    input.poseBindingConfigured = state.poseBindingConfigured;
    return input;
}

} // namespace mousefx::windows
