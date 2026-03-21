#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionPlaceholderScene.h"

#include <algorithm>
#include <cmath>

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

float ResolveSignedUnit(float value) {
    return std::clamp(value, -1.0f, 1.0f);
}

Gdiplus::Color BlendColor(
    const Gdiplus::Color& base,
    const Gdiplus::Color& target,
    float t) {
    const float clamped = ClampUnit(t);
    const auto lerp = [clamped](BYTE a, BYTE b) -> BYTE {
        return static_cast<BYTE>(std::lround(
            static_cast<double>(a) + (static_cast<double>(b) - static_cast<double>(a)) * clamped));
    };
    return Gdiplus::Color(
        lerp(base.GetA(), target.GetA()),
        lerp(base.GetR(), target.GetR()),
        lerp(base.GetG(), target.GetG()),
        lerp(base.GetB(), target.GetB()));
}

std::string NormalizeAsciiId(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

Gdiplus::Color SkinVariantBaseColor(const std::string& skinVariantId) {
    const std::string normalized = NormalizeAsciiId(skinVariantId);
    if (normalized == "strawberry" || normalized == "pink") {
        return Gdiplus::Color(244, 255, 236, 240);
    }
    if (normalized == "midnight" || normalized == "navy") {
        return Gdiplus::Color(244, 228, 236, 250);
    }
    if (normalized == "mint" || normalized == "casual") {
        return Gdiplus::Color(244, 236, 255, 244);
    }
    return Gdiplus::Color(244, 252, 253, 255);
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

Win32MouseCompanionPlaceholderScene BuildWin32MouseCompanionPlaceholderScene(
    const Win32MouseCompanionVisualState& state,
    int width,
    int height) {
    Win32MouseCompanionPlaceholderScene scene{};
    if (width <= 0 || height <= 0) {
        return scene;
    }

    const float intensity = ClampUnit(state.lastActionIntensity);
    const float signedIntensity = ResolveSignedUnit(state.lastActionIntensity);
    const bool follow = state.lastActionName == "follow";
    const bool click = state.lastActionName == "click_react" || state.lastActionName == "drag";
    const bool drag = state.lastActionName == "drag";
    const bool hold = state.lastActionName == "hold_react";
    const bool scroll = state.lastActionName == "scroll_react";
    const float tint = ClampUnit(state.lastHeadTintAmount);
    const Win32MouseCompanionActionSample& clipSample = state.latestActionClipSample;
    const float clickSquash = click ? (0.10f + intensity * 0.14f) : 0.0f;
    const float holdSquat = hold ? (0.10f + intensity * 0.16f) : 0.0f;
    const float scrollLean = scroll ? signedIntensity * 0.12f : 0.0f;

    const MouseCompanionPetPoseFrame& poseFrame = state.latestPoseFrame;
    const MouseCompanionPetPoseSample* leftEarPose = FindPoseSample(poseFrame, kBoneLeftEar);
    const MouseCompanionPetPoseSample* rightEarPose = FindPoseSample(poseFrame, kBoneRightEar);
    const MouseCompanionPetPoseSample* leftHandPose = FindPoseSample(poseFrame, kBoneLeftHand);
    const MouseCompanionPetPoseSample* rightHandPose = FindPoseSample(poseFrame, kBoneRightHand);
    const MouseCompanionPetPoseSample* leftLegPose = FindPoseSample(poseFrame, kBoneLeftLeg);
    const MouseCompanionPetPoseSample* rightLegPose = FindPoseSample(poseFrame, kBoneRightLeg);
    const float poseEarLift = leftEarPose ? leftEarPose->position[1] * 24.0f : 0.0f;
    const float poseEarSpread = leftEarPose ? -leftEarPose->position[0] * 18.0f : 0.0f;
    const float poseRightEarLift = rightEarPose ? rightEarPose->position[1] * 24.0f : 0.0f;
    const float poseRightEarSpread = rightEarPose ? rightEarPose->position[0] * 18.0f : 0.0f;
    const float poseLeftHandLift = leftHandPose ? -leftHandPose->position[1] * 32.0f : 0.0f;
    const float poseRightHandLift = rightHandPose ? -rightHandPose->position[1] * 32.0f : 0.0f;
    const float poseLeftHandSpread = leftHandPose ? leftHandPose->position[0] * 18.0f : 0.0f;
    const float poseRightHandSpread = rightHandPose ? rightHandPose->position[0] * 18.0f : 0.0f;
    const float poseLeftLegShift = leftLegPose ? leftLegPose->position[0] * 20.0f : 0.0f;
    const float poseRightLegShift = rightLegPose ? rightLegPose->position[0] * 20.0f : 0.0f;

    scene.centerX = static_cast<float>(width) * 0.5f;
    const float centerY = static_cast<float>(height) * (hold ? 0.68f : 0.63f);
    const float clipBodyScaleX = clipSample.valid ? std::clamp(clipSample.bodyScaleX, 0.8f, 1.25f) : 1.0f;
    const float clipBodyScaleY = clipSample.valid ? std::clamp(clipSample.bodyScaleY, 0.72f, 1.20f) : 1.0f;
    const float bodyW = static_cast<float>(width) * (hold ? 0.35f : 0.30f) * (1.0f + clickSquash) * clipBodyScaleX;
    const float bodyH = static_cast<float>(height) * (hold ? 0.24f : 0.29f) * (1.0f - clickSquash * 0.55f - holdSquat * 0.35f) * clipBodyScaleY;
    const float headW = static_cast<float>(width) * 0.30f * (1.0f + clickSquash * 0.18f);
    const float headH = static_cast<float>(height) * 0.24f * (1.0f - clickSquash * 0.42f);
    const float clipHeadPitch = clipSample.valid ? clipSample.headPitch : 0.0f;
    const float clipHeadYaw = clipSample.valid ? clipSample.headYaw : 0.0f;
    const float headY = centerY - bodyH * (hold ? 0.76f : 0.82f) - clickSquash * 3.0f + holdSquat * 2.0f - clipHeadPitch * 10.0f;
    const float earLift = (follow ? 7.0f + intensity * 3.0f : 1.5f) + (scroll ? 12.0f * std::abs(signedIntensity) : 0.0f);
    const float earTilt = (follow ? 0.12f + intensity * 0.24f : 0.04f) + (scroll ? signedIntensity * 0.30f : 0.0f);
    const float handLift = hold ? (10.0f + intensity * 12.0f) : (follow ? 3.0f + intensity * 2.0f : 0.0f);
    const float handSpread = drag ? (5.0f + intensity * 7.0f) : (scroll ? std::abs(signedIntensity) * 3.0f : 0.0f);
    const float legShift = follow ? (7.0f + intensity * 7.0f) : 0.0f;
    scene.bodyLeanPx =
        (scrollLean + (clipSample.valid ? clipSample.bodyLean * 0.85f : 0.0f)) * static_cast<float>(width) * 0.30f;
    scene.glowAlpha =
        28.0f + intensity * 18.0f + tint * 16.0f + (scroll ? 12.0f * std::abs(signedIntensity) : 0.0f);
    const float pawY = centerY - bodyH * 0.10f - handLift;

    const Gdiplus::Color variantBase = SkinVariantBaseColor(state.appearanceProfile.skinVariantId);
    scene.bodyFill = BlendColor(
        BlendColor(Gdiplus::Color(236, 245, 247, 255), variantBase, 0.65f),
        Gdiplus::Color(244, 255, 227, 227),
        tint * 0.55f);
    scene.bodyStroke = Gdiplus::Color(220, 89, 109, 141);
    scene.headFill = BlendColor(
        variantBase,
        Gdiplus::Color(252, 255, 214, 214),
        tint * 0.75f);
    scene.earInner = BlendColor(
        Gdiplus::Color(188, 255, 205, 214),
        Gdiplus::Color(212, 255, 150, 158),
        tint);
    scene.accent = scroll && signedIntensity < 0.0f
        ? Gdiplus::Color(220, 125, 154, 255)
        : Gdiplus::Color(220, 116, 204, 178);

    scene.bodyRect = Gdiplus::RectF(
        scene.centerX - bodyW * 0.5f + scene.bodyLeanPx,
        centerY - bodyH * 0.5f,
        bodyW,
        bodyH);
    scene.headRect = Gdiplus::RectF(
        scene.centerX - headW * 0.5f + scene.bodyLeanPx * 0.45f + clipHeadYaw * 10.0f,
        headY - headH * 0.5f,
        headW,
        headH);

    const float earBaseY = scene.headRect.Y + scene.headRect.Height * 0.18f;
    const float earTopY = scene.headRect.Y - scene.headRect.Height * (1.20f + intensity * 0.22f) - earLift - poseEarLift;
    const float rightEarTopY = scene.headRect.Y - scene.headRect.Height * (1.20f + intensity * 0.22f) - earLift - poseRightEarLift;
    const float earBaseOffset = scene.headRect.Width * 0.24f;
    const float earTipOffset = scene.headRect.Width * 0.15f + std::abs(earTilt) * 18.0f;

    scene.leftEar = {{
        { scene.centerX - earBaseOffset, earBaseY },
        { scene.centerX - earBaseOffset - scene.headRect.Width * 0.08f - earTilt * 16.0f - poseEarSpread, scene.headRect.Y - scene.headRect.Height * 0.16f },
        { scene.centerX - earTipOffset - scene.headRect.Width * 0.02f - poseEarSpread, earTopY },
        { scene.centerX - earBaseOffset + scene.headRect.Width * 0.03f, scene.headRect.Y + scene.headRect.Height * 0.02f },
    }};
    scene.rightEar = {{
        { scene.centerX + earBaseOffset, earBaseY },
        { scene.centerX + earBaseOffset + scene.headRect.Width * 0.08f - earTilt * 16.0f + poseRightEarSpread, scene.headRect.Y - scene.headRect.Height * 0.16f },
        { scene.centerX + earTipOffset + scene.headRect.Width * 0.02f + poseRightEarSpread, rightEarTopY },
        { scene.centerX + earBaseOffset - scene.headRect.Width * 0.03f, scene.headRect.Y + scene.headRect.Height * 0.02f },
    }};

    scene.leftHandRect = Gdiplus::RectF(
        scene.centerX - bodyW * 0.50f + scene.bodyLeanPx - handSpread + poseLeftHandSpread,
        pawY - poseLeftHandLift,
        bodyW * 0.20f,
        bodyH * 0.34f);
    scene.rightHandRect = Gdiplus::RectF(
        scene.centerX + bodyW * 0.30f + scene.bodyLeanPx + handSpread + poseRightHandSpread,
        pawY - poseRightHandLift,
        bodyW * 0.20f,
        bodyH * 0.34f);
    scene.leftLegRect = Gdiplus::RectF(
        scene.centerX - bodyW * 0.34f - legShift * 0.25f + scene.bodyLeanPx * 0.28f + poseLeftLegShift,
        centerY + bodyH * 0.34f,
        bodyW * 0.18f,
        bodyH * 0.24f);
    scene.rightLegRect = Gdiplus::RectF(
        scene.centerX + bodyW * 0.16f + legShift * 0.25f + scene.bodyLeanPx * 0.28f + poseRightLegShift,
        centerY + bodyH * 0.34f,
        bodyW * 0.18f,
        bodyH * 0.24f);

    scene.modelAssetAvailable = state.modelAssetAvailable;
    scene.actionLibraryAvailable = state.actionLibraryAvailable;
    scene.poseBadgeVisible = state.poseBindingConfigured || state.poseFrameAvailable;
    scene.accessoryVisible = !state.appearanceProfile.enabledAccessoryIds.empty();
    scene.eyeColor = Gdiplus::Color(220, 78, 84, 111);
    scene.mouthColor = Gdiplus::Color(190, 98, 107, 140);
    scene.blushColor = BlendColor(
        Gdiplus::Color(72, 255, 184, 196),
        Gdiplus::Color(128, 255, 112, 122),
        tint);
    scene.eyeHeight = hold ? 4.5f : (click ? 3.5f : 8.0f);
    return scene;
}

} // namespace mousefx::windows
