#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelSceneAdapterProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetResources.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <array>

namespace mousefx::windows {
namespace {

bool IsSupportedModelSceneSourceFormat(const std::string& format) {
    static constexpr std::array<const char*, 4> kSupportedFormats = {
        "glb",
        "gltf",
        "vrm",
        "fbx",
    };
    for (const char* candidate : kSupportedFormats) {
        if (format == candidate) {
            return true;
        }
    }
    return false;
}

std::string BuildModelSceneAdapterBrief(
    const std::string& seamState,
    const std::string& sourceFormat,
    const std::string& adapterMode) {
    const std::string normalizedState =
        seamState.empty() ? "preview_only" : seamState;
    const std::string normalizedFormat =
        sourceFormat.empty() ? "unknown" : sourceFormat;
    const std::string normalizedAdapter =
        adapterMode.empty() ? "runtime_only" : adapterMode;
    return normalizedState + "/" + normalizedFormat + "/" + normalizedAdapter;
}

} // namespace

Win32MouseCompanionRealRendererModelSceneAdapterProfile
BuildWin32MouseCompanionRealRendererModelSceneAdapterProfile(
    const Win32MouseCompanionRealRendererAssetResources& assets,
    const std::string& adapterMode,
    bool poseFrameAvailable,
    bool poseBindingConfigured) {
    Win32MouseCompanionRealRendererModelSceneAdapterProfile profile{};
    profile.sourceReady = assets.modelReady;
    profile.sourceFormatSupported =
        assets.modelReady && IsSupportedModelSceneSourceFormat(assets.modelSourceFormat);
    profile.poseSamplingReady =
        profile.sourceFormatSupported &&
        (poseFrameAvailable || adapterMode == "pose_unbound" || adapterMode == "pose_bound");
    profile.boundPoseReady =
        profile.sourceFormatSupported &&
        poseBindingConfigured &&
        adapterMode == "pose_bound";

    if (!profile.sourceFormatSupported) {
        profile.seamState = "preview_only";
        profile.seamReadiness = 0.0f;
    } else if (profile.boundPoseReady) {
        profile.seamState = "pose_bound_preview_ready";
        profile.seamReadiness = 1.0f;
    } else if (profile.poseSamplingReady) {
        profile.seamState = "pose_stub_ready";
        profile.seamReadiness = 0.72f;
    } else {
        profile.seamState = "asset_stub_ready";
        profile.seamReadiness = 0.42f;
    }

    profile.brief = BuildModelSceneAdapterBrief(
        profile.seamState,
        assets.modelSourceFormat,
        adapterMode);
    return profile;
}

Win32MouseCompanionRealRendererModelSceneAdapterProfile
BuildWin32MouseCompanionRealRendererModelSceneAdapterProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    if (runtime.assets == nullptr) {
        return Win32MouseCompanionRealRendererModelSceneAdapterProfile{};
    }
    return BuildWin32MouseCompanionRealRendererModelSceneAdapterProfile(
        *runtime.assets,
        runtime.sceneRuntimeAdapterMode,
        runtime.poseFrameAvailable,
        runtime.poseBindingConfigured);
}

} // namespace mousefx::windows
