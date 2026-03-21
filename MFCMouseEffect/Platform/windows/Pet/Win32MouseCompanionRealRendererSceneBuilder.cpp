#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneBuilder.h"

#include <algorithm>
#include <cmath>

namespace mousefx::windows {
namespace {

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float ClampSigned(float value) {
    return std::clamp(value, -1.0f, 1.0f);
}

Gdiplus::Color MakeColor(BYTE a, BYTE r, BYTE g, BYTE b) {
    return Gdiplus::Color(a, r, g, b);
}

Gdiplus::Color PickBodyFill(const std::string& skinVariantId) {
    if (skinVariantId == "cream") {
        return MakeColor(255, 255, 243, 212);
    }
    if (skinVariantId == "night") {
        return MakeColor(255, 73, 87, 126);
    }
    if (skinVariantId == "strawberry") {
        return MakeColor(255, 255, 228, 235);
    }
    return MakeColor(255, 239, 244, 255);
}

Gdiplus::Color Darken(const Gdiplus::Color& color, float factor) {
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    const auto scale = [clamped](BYTE c) -> BYTE {
        return static_cast<BYTE>(std::lround(static_cast<float>(c) * (1.0f - clamped)));
    };
    return MakeColor(color.GetA(), scale(color.GetR()), scale(color.GetG()), scale(color.GetB()));
}

std::array<Gdiplus::PointF, 5> BuildStarPoints(float centerX, float centerY, float outerRadius, float innerRadius) {
    std::array<Gdiplus::PointF, 5> points{};
    for (size_t i = 0; i < points.size(); ++i) {
        const float angle = -3.1415926f * 0.5f + static_cast<float>(i) * (3.1415926f * 2.0f / 5.0f);
        const float x = centerX + std::cos(angle) * outerRadius;
        const float y = centerY + std::sin(angle) * outerRadius;
        points[i] = Gdiplus::PointF(x, y - innerRadius * 0.12f);
    }
    return points;
}

} // namespace

Win32MouseCompanionRealRendererScene BuildWin32MouseCompanionRealRendererScene(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    int width,
    int height) {
    Win32MouseCompanionRealRendererScene scene{};
    if (width <= 0 || height <= 0 || !runtime.assets) {
        return scene;
    }

    const float actionIntensity = Clamp01(runtime.actionIntensity);
    const float reactiveIntensity = Clamp01(runtime.reactiveActionIntensity);
    const float scrollIntensity = std::abs(ClampSigned(runtime.scrollSignedIntensity));
    const float poseEarLift = runtime.leftEarPose ? runtime.leftEarPose->position[1] * 22.0f : 0.0f;
    const float poseEarSpread = runtime.leftEarPose ? -runtime.leftEarPose->position[0] * 18.0f : 0.0f;
    const float poseRightEarLift = runtime.rightEarPose ? runtime.rightEarPose->position[1] * 22.0f : 0.0f;
    const float poseRightEarSpread = runtime.rightEarPose ? runtime.rightEarPose->position[0] * 18.0f : 0.0f;
    const float poseLeftHandLift = runtime.leftHandPose ? -runtime.leftHandPose->position[1] * 26.0f : 0.0f;
    const float poseRightHandLift = runtime.rightHandPose ? -runtime.rightHandPose->position[1] * 26.0f : 0.0f;
    const float poseLeftLegShift = runtime.leftLegPose ? runtime.leftLegPose->position[0] * 18.0f : 0.0f;
    const float poseRightLegShift = runtime.rightLegPose ? runtime.rightLegPose->position[0] * 18.0f : 0.0f;

    const float glowBoost = std::max({actionIntensity, reactiveIntensity, scrollIntensity});
    const float facingOffset = runtime.facingSign * std::min(runtime.facingMomentumPx * 0.03f, 12.0f);
    const float bodyWidth = static_cast<float>(width) * (runtime.hold ? 0.34f : 0.31f);
    const float bodyHeight = static_cast<float>(height) * (runtime.hold ? 0.27f : 0.31f);
    const float headWidth = bodyWidth * 0.78f;
    const float headHeight = bodyHeight * 0.66f;
    const float clickSquash = runtime.click ? (0.06f + actionIntensity * 0.10f) : 0.0f;
    const float dragLean = runtime.drag ? runtime.facingSign * (4.0f + actionIntensity * 5.0f) : 0.0f;
    const float scrollLean = runtime.scrollSignedIntensity * 7.5f;
    const float earLift = (runtime.follow ? 8.0f + actionIntensity * 4.0f : 2.0f) +
        (runtime.scroll ? scrollIntensity * 10.0f : 0.0f);
    const float earSwing = runtime.facingSign * (runtime.follow ? 5.0f + actionIntensity * 4.0f : 1.8f);
    const float handLift = runtime.hold ? (11.0f + actionIntensity * 10.0f) : (runtime.follow ? 3.0f : 0.0f);
    const float legStride = runtime.follow ? (6.0f + actionIntensity * 6.0f) : 0.0f;

    const auto baseBody = PickBodyFill(runtime.assets->appearanceProfileSkinVariantId);
    scene.centerX = static_cast<float>(width) * 0.5f + facingOffset;
    scene.centerY = static_cast<float>(height) * (runtime.hold ? 0.60f : 0.58f);
    scene.facingSign = runtime.facingSign;
    scene.bodyTiltDeg = scrollLean + dragLean;
    scene.glowAlpha = 28.0f + glowBoost * 54.0f + runtime.headTintAmount * 18.0f;
    scene.glowColor = MakeColor(255, 82, 170, 255);
    scene.bodyFill = baseBody;
    scene.bodyFillRear = Darken(baseBody, 0.12f);
    scene.bodyStroke = MakeColor(255, 70, 98, 152);
    scene.headFill = MakeColor(255, 255, 250, 246);
    scene.headFillRear = MakeColor(255, 241, 234, 232);
    scene.earFill = MakeColor(255, 255, 247, 235);
    scene.earFillRear = MakeColor(255, 235, 226, 214);
    scene.earInner = MakeColor(255, 255, 201, 214);
    scene.eyeFill = MakeColor(255, 38, 44, 62);
    scene.mouthFill = MakeColor(255, 106, 84, 114);
    scene.blushFill = MakeColor(150, 255, 171, 194);
    scene.tailFill = Darken(baseBody, 0.08f);
    scene.accentFill = MakeColor(255, 111, 219, 255);
    scene.pedestalFill = MakeColor(105, 59, 77, 115);
    scene.badgeReadyFill = MakeColor(255, 111, 229, 178);
    scene.badgePendingFill = MakeColor(255, 255, 189, 97);
    scene.accessoryFill = MakeColor(255, 255, 221, 97);
    scene.accessoryStroke = MakeColor(255, 144, 98, 38);

    scene.glowRect = Gdiplus::RectF(
        scene.centerX - bodyWidth * 0.98f,
        scene.centerY - bodyHeight * 0.90f,
        bodyWidth * 1.96f,
        bodyHeight * 1.95f);
    scene.shadowRect = Gdiplus::RectF(
        scene.centerX - bodyWidth * 0.58f,
        scene.centerY + bodyHeight * 0.50f,
        bodyWidth * 1.16f,
        bodyHeight * 0.22f);
    scene.bodyRect = Gdiplus::RectF(
        scene.centerX - bodyWidth * 0.50f,
        scene.centerY - bodyHeight * 0.50f + clickSquash * 6.0f,
        bodyWidth * (1.0f + clickSquash * 0.40f),
        bodyHeight * (1.0f - clickSquash * 0.32f));
    scene.chestRect = Gdiplus::RectF(
        scene.centerX - bodyWidth * 0.18f,
        scene.centerY - bodyHeight * 0.04f,
        bodyWidth * 0.36f,
        bodyHeight * 0.28f);
    scene.headRect = Gdiplus::RectF(
        scene.centerX - headWidth * 0.50f + dragLean * 0.18f,
        scene.bodyRect.Y - headHeight * 0.48f - actionIntensity * 4.0f,
        headWidth,
        headHeight);
    scene.tailRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - bodyWidth * 0.12f - scene.facingSign * bodyWidth * 0.34f,
        scene.centerY - bodyHeight * 0.02f,
        bodyWidth * 0.44f,
        bodyHeight * 0.18f);
    scene.pedestalRect = Gdiplus::RectF(
        scene.centerX - bodyWidth * 0.58f,
        scene.shadowRect.GetBottom() - bodyHeight * 0.04f,
        bodyWidth * 1.16f,
        bodyHeight * 0.20f);

    const float earBaseY = scene.headRect.Y + scene.headRect.Height * 0.08f;
    const float earTipY = scene.headRect.Y - scene.headRect.Height * 0.42f - earLift - poseEarLift;
    const float rightEarTipY = scene.headRect.Y - scene.headRect.Height * 0.42f - earLift - poseRightEarLift;
    const float earBaseOffset = scene.headRect.Width * 0.22f;
    scene.leftEar = {{
        Gdiplus::PointF(scene.centerX - earBaseOffset, earBaseY),
        Gdiplus::PointF(scene.centerX - earBaseOffset - scene.headRect.Width * 0.10f + poseEarSpread - earSwing, scene.headRect.Y + scene.headRect.Height * 0.06f),
        Gdiplus::PointF(scene.centerX - scene.headRect.Width * 0.24f + poseEarSpread - earSwing * 0.7f, earTipY),
        Gdiplus::PointF(scene.centerX - scene.headRect.Width * 0.06f, scene.headRect.Y + scene.headRect.Height * 0.04f),
    }};
    scene.rightEar = {{
        Gdiplus::PointF(scene.centerX + earBaseOffset, earBaseY),
        Gdiplus::PointF(scene.centerX + earBaseOffset + scene.headRect.Width * 0.10f + poseRightEarSpread - earSwing, scene.headRect.Y + scene.headRect.Height * 0.06f),
        Gdiplus::PointF(scene.centerX + scene.headRect.Width * 0.24f + poseRightEarSpread - earSwing * 0.5f, rightEarTipY),
        Gdiplus::PointF(scene.centerX + scene.headRect.Width * 0.06f, scene.headRect.Y + scene.headRect.Height * 0.04f),
    }};

    scene.leftHandRect = Gdiplus::RectF(
        scene.bodyRect.X - bodyWidth * 0.06f,
        scene.centerY - bodyHeight * 0.02f - handLift - poseLeftHandLift,
        bodyWidth * 0.18f,
        bodyHeight * 0.30f);
    scene.rightHandRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - bodyWidth * 0.12f,
        scene.centerY - bodyHeight * 0.02f - handLift - poseRightHandLift,
        bodyWidth * 0.18f,
        bodyHeight * 0.30f);
    scene.leftLegRect = Gdiplus::RectF(
        scene.centerX - bodyWidth * 0.26f - legStride * 0.24f + poseLeftLegShift,
        scene.bodyRect.GetBottom() - bodyHeight * 0.03f,
        bodyWidth * 0.17f,
        bodyHeight * 0.22f);
    scene.rightLegRect = Gdiplus::RectF(
        scene.centerX + bodyWidth * 0.09f + legStride * 0.24f + poseRightLegShift,
        scene.bodyRect.GetBottom() - bodyHeight * 0.03f,
        bodyWidth * 0.17f,
        bodyHeight * 0.22f);

    const float eyeOpen = runtime.hold ? 0.28f : (runtime.click ? 0.42f : 1.0f);
    const float eyeH = std::max(3.0f, scene.headRect.Height * 0.12f * eyeOpen);
    scene.leftEyeRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * 0.20f,
        scene.headRect.Y + scene.headRect.Height * 0.43f,
        5.0f,
        eyeH);
    scene.rightEyeRect = Gdiplus::RectF(
        scene.centerX + scene.headRect.Width * 0.20f - 5.0f,
        scene.headRect.Y + scene.headRect.Height * 0.43f,
        5.0f,
        eyeH);
    scene.noseRect = Gdiplus::RectF(
        scene.centerX - 2.0f,
        scene.headRect.Y + scene.headRect.Height * 0.54f,
        4.0f,
        3.0f);
    scene.mouthRect = Gdiplus::RectF(
        scene.centerX - 6.0f,
        scene.headRect.Y + scene.headRect.Height * 0.60f,
        12.0f,
        7.0f + reactiveIntensity * 3.0f);
    scene.leftBlushRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * 0.32f,
        scene.headRect.Y + scene.headRect.Height * 0.60f,
        9.0f,
        5.0f);
    scene.rightBlushRect = Gdiplus::RectF(
        scene.centerX + scene.headRect.Width * 0.32f - 9.0f,
        scene.headRect.Y + scene.headRect.Height * 0.60f,
        9.0f,
        5.0f);

    const float badgeY = scene.pedestalRect.GetBottom() + bodyHeight * 0.02f;
    const float badgeW = bodyWidth * 0.18f;
    const float badgeH = bodyHeight * 0.13f;
    for (size_t i = 0; i < scene.laneBadgeRects.size(); ++i) {
        const float x = scene.centerX - badgeW * 1.8f + static_cast<float>(i) * (badgeW * 1.4f);
        scene.laneBadgeRects[i] = Gdiplus::RectF(x, badgeY, badgeW, badgeH);
    }
    scene.laneReady = {
        runtime.assets->modelReady,
        runtime.assets->actionLibraryReady,
        runtime.assets->appearanceProfileReady,
    };

    scene.poseBadgeVisible = runtime.poseFrameAvailable || runtime.poseBindingConfigured;
    scene.poseBadgeRect = Gdiplus::RectF(
        scene.headRect.GetRight() - scene.headRect.Width * 0.12f,
        scene.headRect.Y - scene.headRect.Height * 0.03f,
        scene.headRect.Width * 0.22f,
        scene.headRect.Width * 0.22f);

    scene.accessoryVisible = !runtime.assets->appearanceAccessoryIds.empty();
    if (scene.accessoryVisible) {
        scene.accessoryStar = BuildStarPoints(
            scene.headRect.GetRight() - scene.headRect.Width * 0.08f,
            scene.headRect.Y + scene.headRect.Height * 0.18f,
            scene.headRect.Width * 0.11f,
            scene.headRect.Width * 0.05f);
    }
    return scene;
}

} // namespace mousefx::windows
