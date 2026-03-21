#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRendererRuntime.h"

#include "MouseFx/Utils/TimeUtils.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

constexpr int kBoneLeftEar = 0;
constexpr int kBoneRightEar = 1;
constexpr int kBoneLeftHand = 2;
constexpr int kBoneRightHand = 3;
constexpr int kBoneLeftLeg = 4;
constexpr int kBoneRightLeg = 5;

float ClampUnit(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float ClampSignedUnit(float value) {
    return std::clamp(value, -1.0f, 1.0f);
}

const MouseCompanionPetPoseSample* FindPoseSample(
    const MouseCompanionPetPoseFrame& frame,
    int boneIndex) {
    for (const auto& sample : frame.samples) {
        if (sample.boneIndex == boneIndex) {
            return &sample;
        }
    }
    return nullptr;
}

} // namespace

Win32MouseCompanionRendererRuntime BuildWin32MouseCompanionRendererRuntime(
    const Win32MouseCompanionRendererInput& input) {
    Win32MouseCompanionRendererRuntime runtime{};
    runtime.config = &input.config;
    runtime.appearanceProfile = &input.appearanceProfile;
    runtime.clipSample = &input.latestActionClipSample;
    runtime.poseFrame = &input.latestPoseFrame;
    runtime.actionIntensity = ClampUnit(input.actionIntensity);
    runtime.signedActionIntensity = ClampSignedUnit(input.actionIntensity);
    runtime.headTintAmount = ClampUnit(input.headTintAmount);
    runtime.facingSign = (input.facingDirection >= 0) ? 1.0f : -1.0f;
    runtime.facingMomentumPx = input.facingMomentumPx;
    runtime.scrollSignedIntensity = ClampSignedUnit(input.scrollSignedIntensity);
    runtime.reactiveActionIntensity = ClampUnit(input.reactiveActionIntensity);
    runtime.nowMs = NowMs();
    runtime.poseSampleTickMs = input.poseSampleTickMs;
    runtime.clickTriggerTickMs = input.clickTriggerTickMs;
    runtime.holdTriggerTickMs = input.holdTriggerTickMs;
    runtime.scrollTriggerTickMs = input.scrollTriggerTickMs;
    runtime.follow = input.actionName == "follow";
    runtime.drag = input.actionName == "drag";
    runtime.hold = input.actionName == "hold_react";
    runtime.scroll = input.actionName == "scroll_react";
    runtime.click = input.actionName == "click_react" || runtime.drag;
    runtime.modelAssetAvailable = input.modelAssetAvailable;
    runtime.actionLibraryAvailable = input.actionLibraryAvailable;
    runtime.poseFrameAvailable = input.poseFrameAvailable;
    runtime.poseBindingConfigured = input.poseBindingConfigured;
    runtime.leftEarPose = FindPoseSample(input.latestPoseFrame, kBoneLeftEar);
    runtime.rightEarPose = FindPoseSample(input.latestPoseFrame, kBoneRightEar);
    runtime.leftHandPose = FindPoseSample(input.latestPoseFrame, kBoneLeftHand);
    runtime.rightHandPose = FindPoseSample(input.latestPoseFrame, kBoneRightHand);
    runtime.leftLegPose = FindPoseSample(input.latestPoseFrame, kBoneLeftLeg);
    runtime.rightLegPose = FindPoseSample(input.latestPoseFrame, kBoneRightLeg);
    return runtime;
}

} // namespace mousefx::windows
