#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionVisualRuntime.h"

#include <filesystem>

namespace mousefx::windows {
namespace {

std::string ResolveActionName(int actionCode) {
    switch (actionCode) {
    case 1:
        return "follow";
    case 2:
        return "click_react";
    case 3:
        return "drag";
    case 4:
        return "hold_react";
    case 5:
        return "scroll_react";
    default:
        return "idle";
    }
}

} // namespace

void ResetWin32MouseCompanionVisualState(
    Win32MouseCompanionVisualState* state,
    const MouseCompanionPetRuntimeConfig& config,
    bool active) {
    if (!state) {
        return;
    }
    *state = {};
    state->config = config;
    state->active = active;
    state->visible = false;
    state->hasLastPoint = false;
}

void ApplyWin32MouseCompanionVisualConfig(
    Win32MouseCompanionVisualState* state,
    const MouseCompanionPetRuntimeConfig& config) {
    if (!state) {
        return;
    }
    state->config = config;
    state->active = config.enabled;
}

bool ApplyWin32MouseCompanionModelAssetState(
    Win32MouseCompanionVisualState* state,
    const std::string& modelPath) {
    if (!state) {
        return false;
    }
    state->lastModelPath = modelPath;
    state->modelAssetAvailable =
        !modelPath.empty() && std::filesystem::exists(std::filesystem::path(modelPath));
    return state->modelAssetAvailable;
}

void ApplyWin32MouseCompanionAppearanceState(
    Win32MouseCompanionVisualState* state,
    Win32MouseCompanionAppearanceProfile profile,
    bool loaded) {
    if (!state) {
        return;
    }
    state->appearanceProfile = loaded ? std::move(profile) : Win32MouseCompanionAppearanceProfile{};
}

void ApplyWin32MouseCompanionPoseBindingState(
    Win32MouseCompanionVisualState* state,
    bool configured) {
    if (!state) {
        return;
    }
    state->poseBindingConfigured = configured && state->active;
}

void ApplyWin32MouseCompanionFollowPoint(
    Win32MouseCompanionVisualState* state,
    const ScreenPoint& pt) {
    if (!state || !state->active) {
        return;
    }
    state->lastPoint = pt;
    state->hasLastPoint = true;
}

void ApplyWin32MouseCompanionHostUpdate(
    Win32MouseCompanionVisualState* state,
    const PetVisualHostUpdate& update) {
    if (!state || !state->active) {
        return;
    }
    state->lastPoint = update.pt;
    state->hasLastPoint = true;
    state->lastActionCode = update.actionCode;
    state->lastActionIntensity = update.actionIntensity;
    state->lastHeadTintAmount = update.headTintAmount;
    state->lastActionName = ResolveActionName(update.actionCode);
}

void ApplyWin32MouseCompanionPoseFrame(
    Win32MouseCompanionVisualState* state,
    const MouseCompanionPetPoseFrame& poseFrame) {
    if (!state || !state->active) {
        return;
    }
    state->lastPoseSampleTickMs = poseFrame.sampleTickMs;
    state->latestPoseFrame = poseFrame;
    state->lastActionName = poseFrame.actionName;
    state->lastActionIntensity = poseFrame.actionIntensity;
    state->lastHeadTintAmount = poseFrame.headTintAmount;
    state->poseFrameAvailable = !poseFrame.samples.empty();
}

} // namespace mousefx::windows
