#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererFaceBuilder.h"

#include <algorithm>

namespace mousefx::windows {

void BuildWin32MouseCompanionRealRendererFace(
    const Win32MouseCompanionRealRendererMotionProfile& profile,
    const Win32MouseCompanionRealRendererStyleProfile& style,
    Win32MouseCompanionRealRendererScene& scene) {
    const float eyeH = std::max(3.0f, scene.headRect.Height * style.eyeHeightRatio * profile.eyeOpen);
    scene.leftEyeRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.eyeLeftXRatio,
        scene.headRect.Y + scene.headRect.Height * style.eyeYRatio,
        style.eyeWidthPx,
        eyeH);
    scene.rightEyeRect = Gdiplus::RectF(
        scene.centerX + scene.headRect.Width * style.eyeRightXRatio - style.eyeWidthPx,
        scene.headRect.Y + scene.headRect.Height * style.eyeYRatio,
        style.eyeWidthPx,
        eyeH);
    scene.noseRect = Gdiplus::RectF(
        scene.centerX - style.noseWidthPx * 0.5f,
        scene.headRect.Y + scene.headRect.Height * style.noseYRatio,
        style.noseWidthPx,
        style.noseHeightPx);
    scene.mouthRect = Gdiplus::RectF(
        scene.centerX - style.mouthWidthPx * 0.5f,
        scene.headRect.Y + scene.headRect.Height * style.mouthYRatio,
        style.mouthWidthPx,
        style.mouthHeightBasePx + profile.reactiveIntensity * style.mouthReactiveHeightPx);
    const float browY = scene.headRect.Y + scene.headRect.Height * style.browYRatio;
    scene.leftBrowStart = Gdiplus::PointF(
        scene.centerX - scene.headRect.Width * style.leftBrowStartXRatio,
        browY + profile.browLift - profile.browTilt * style.leftBrowStartTiltScale);
    scene.leftBrowEnd = Gdiplus::PointF(
        scene.centerX - scene.headRect.Width * style.leftBrowEndXRatio,
        browY + profile.browLift + profile.browTilt * style.leftBrowEndTiltScale);
    scene.rightBrowStart = Gdiplus::PointF(
        scene.centerX + scene.headRect.Width * style.rightBrowStartXRatio,
        browY + profile.browLift - profile.browTilt * style.rightBrowStartTiltScale);
    scene.rightBrowEnd = Gdiplus::PointF(
        scene.centerX + scene.headRect.Width * style.rightBrowEndXRatio,
        browY + profile.browLift + profile.browTilt * style.rightBrowEndTiltScale);
    scene.mouthStartDeg = profile.mouthStartDeg;
    scene.mouthSweepDeg = profile.mouthSweepDeg;
    scene.mouthStrokeWidth = profile.mouthStrokeWidth;
    scene.leftBlushRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.blushXRatio,
        scene.headRect.Y + scene.headRect.Height * style.blushYRatio,
        style.blushWidthPx,
        style.blushHeightPx);
    scene.rightBlushRect = Gdiplus::RectF(
        scene.centerX + scene.headRect.Width * style.blushXRatio - style.blushWidthPx,
        scene.headRect.Y + scene.headRect.Height * style.blushYRatio,
        style.blushWidthPx,
        style.blushHeightPx);
}

} // namespace mousefx::windows
