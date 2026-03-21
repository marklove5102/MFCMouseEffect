#pragma once

#include <string>

#include "MouseFx/Core/Control/MouseCompanionPluginV1Types.h"
#include "Platform/windows/Pet/Win32MouseCompanionActionLibrary.h"
#include "Platform/windows/Pet/Win32MouseCompanionAppearanceProfile.h"

namespace mousefx::windows {

struct Win32MouseCompanionRendererInput {
    MouseCompanionPetRuntimeConfig config{};
    std::string modelPath;
    std::string actionLibraryPath;
    std::string actionName{"idle"};
    std::string reactiveActionName{"idle"};
    int actionCode{0};
    int facingDirection{1};
    float actionIntensity{0.0f};
    float headTintAmount{0.0f};
    float facingMomentumPx{0.0f};
    float scrollSignedIntensity{0.0f};
    float reactiveActionIntensity{0.0f};
    uint64_t poseSampleTickMs{0};
    uint64_t clickTriggerTickMs{0};
    uint64_t holdTriggerTickMs{0};
    uint64_t scrollTriggerTickMs{0};
    MouseCompanionPetPoseFrame latestPoseFrame{};
    Win32MouseCompanionActionSample latestActionClipSample{};
    Win32MouseCompanionAppearanceProfile appearanceProfile{};
    bool modelAssetAvailable{false};
    bool actionLibraryAvailable{false};
    bool poseFrameAvailable{false};
    bool poseBindingConfigured{false};
};

} // namespace mousefx::windows
