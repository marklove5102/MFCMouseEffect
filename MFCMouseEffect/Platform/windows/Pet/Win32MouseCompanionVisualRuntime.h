#pragma once

#include <string>

#include "MouseFx/Core/Control/IPetVisualHost.h"
#include "Platform/windows/Pet/Win32MouseCompanionAppearanceProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionVisualState.h"

namespace mousefx::windows {

void ResetWin32MouseCompanionVisualState(
    Win32MouseCompanionVisualState* state,
    const MouseCompanionPetRuntimeConfig& config,
    bool active);

void ApplyWin32MouseCompanionVisualConfig(
    Win32MouseCompanionVisualState* state,
    const MouseCompanionPetRuntimeConfig& config);

bool ApplyWin32MouseCompanionModelAssetState(
    Win32MouseCompanionVisualState* state,
    const std::string& modelPath);

void ApplyWin32MouseCompanionAppearanceState(
    Win32MouseCompanionVisualState* state,
    Win32MouseCompanionAppearanceProfile profile,
    bool loaded);

void ApplyWin32MouseCompanionPoseBindingState(
    Win32MouseCompanionVisualState* state,
    bool configured);

void ApplyWin32MouseCompanionFollowPoint(
    Win32MouseCompanionVisualState* state,
    const ScreenPoint& pt);

void ApplyWin32MouseCompanionHostUpdate(
    Win32MouseCompanionVisualState* state,
    const PetVisualHostUpdate& update);

void ApplyWin32MouseCompanionPoseFrame(
    Win32MouseCompanionVisualState* state,
    const MouseCompanionPetPoseFrame& poseFrame);

} // namespace mousefx::windows
