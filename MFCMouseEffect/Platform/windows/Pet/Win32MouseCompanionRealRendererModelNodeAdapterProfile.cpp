#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeAdapterProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <cstdio>

namespace mousefx::windows {
namespace {

float AveragePoseAxis(
    const MouseCompanionPetPoseSample* first,
    const MouseCompanionPetPoseSample* second,
    size_t axis) {
    float sum = 0.0f;
    float count = 0.0f;
    if (first) {
        sum += first->position[axis];
        count += 1.0f;
    }
    if (second) {
        sum += second->position[axis];
        count += 1.0f;
    }
    return count > 0.0f ? sum / count : 0.0f;
}

float ResolveModelNodeInfluence(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    const float poseInfluence = runtime.poseAdapterProfile.influence;
    const std::string& seamState = runtime.modelSceneAdapterProfile.seamState;
    if (seamState == "pose_bound_preview_ready") {
        return poseInfluence;
    }
    if (seamState == "pose_stub_ready") {
        return poseInfluence * 0.72f;
    }
    if (seamState == "asset_stub_ready") {
        return poseInfluence * 0.35f;
    }
    return 0.0f;
}

std::string BuildModelNodeAdapterBrief(
    const std::string& seamState,
    float influence) {
    char buffer[96];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s/%.2f",
        seamState.empty() ? "preview_only" : seamState.c_str(),
        influence);
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererModelNodeAdapterProfile
BuildWin32MouseCompanionRealRendererModelNodeAdapterProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererModelNodeAdapterProfile profile{};
    profile.influence = ResolveModelNodeInfluence(runtime);

    const float handReachX =
        AveragePoseAxis(runtime.leftHandPose, runtime.rightHandPose, 0);
    const float handLiftY =
        AveragePoseAxis(runtime.leftHandPose, runtime.rightHandPose, 1);
    const float legReachX =
        AveragePoseAxis(runtime.leftLegPose, runtime.rightLegPose, 0);
    const float legLiftY =
        AveragePoseAxis(runtime.leftLegPose, runtime.rightLegPose, 1);

    profile.centerOffsetX =
        (handReachX * 0.045f + legReachX * 0.030f) * profile.influence;
    profile.centerOffsetY =
        (-handLiftY * 0.032f - legLiftY * 0.016f) * profile.influence;
    profile.faceOffsetX = handReachX * 0.022f * profile.influence;
    profile.faceOffsetY = -handLiftY * 0.030f * profile.influence;
    profile.overlayOffsetX =
        (handReachX * 0.030f + legReachX * 0.018f) * profile.influence;
    profile.overlayOffsetY = (-handLiftY * 0.030f) * profile.influence;
    profile.adornmentOffsetX =
        (handReachX * 0.020f + legReachX * 0.015f) * profile.influence;
    profile.adornmentOffsetY =
        (-handLiftY * 0.018f - legLiftY * 0.010f) * profile.influence;
    profile.groundingOffsetX = legReachX * 0.022f * profile.influence;
    profile.groundingOffsetY = -legLiftY * 0.018f * profile.influence;
    profile.whiskerBias = handReachX * 0.16f * profile.influence;
    profile.blushLift = (-handLiftY) * 1.2f * profile.influence;
    profile.brief = BuildModelNodeAdapterBrief(
        runtime.modelSceneAdapterProfile.seamState,
        profile.influence);
    return profile;
}

} // namespace mousefx::windows
