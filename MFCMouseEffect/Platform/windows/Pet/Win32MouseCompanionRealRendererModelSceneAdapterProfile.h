#pragma once

#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererAssetResources;
struct Win32MouseCompanionRealRendererSceneRuntime;

struct Win32MouseCompanionRealRendererModelSceneAdapterProfile final {
    bool sourceReady{false};
    bool sourceFormatSupported{false};
    bool poseSamplingReady{false};
    bool boundPoseReady{false};
    float seamReadiness{0.0f};
    std::string seamState{"preview_only"};
    std::string brief{"preview_only/unknown/runtime_only"};
};

Win32MouseCompanionRealRendererModelSceneAdapterProfile
BuildWin32MouseCompanionRealRendererModelSceneAdapterProfile(
    const Win32MouseCompanionRealRendererAssetResources& assets,
    const std::string& adapterMode,
    bool poseFrameAvailable,
    bool poseBindingConfigured);

Win32MouseCompanionRealRendererModelSceneAdapterProfile
BuildWin32MouseCompanionRealRendererModelSceneAdapterProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

} // namespace mousefx::windows
