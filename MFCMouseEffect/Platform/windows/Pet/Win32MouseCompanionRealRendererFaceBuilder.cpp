#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererFaceBuilder.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

void SetWhiskerPair(
    float anchorX,
    float anchorY,
    float direction,
    float length,
    float verticalOffset,
    float spread,
    float tilt,
    Gdiplus::PointF& start,
    Gdiplus::PointF& end) {
    start = Gdiplus::PointF(anchorX, anchorY + verticalOffset);
    end = Gdiplus::PointF(
        anchorX + direction * length,
        anchorY + verticalOffset + verticalOffset * spread + tilt);
}

} // namespace

void BuildWin32MouseCompanionRealRendererFace(
    const Win32MouseCompanionRealRendererMotionProfile& profile,
    const Win32MouseCompanionRealRendererStyleProfile& style,
    Win32MouseCompanionRealRendererScene& scene) {
    const float eyeH = std::max(3.0f, scene.headRect.Height * style.eyeHeightRatio * profile.eyeOpen);
    const float pupilH = std::max(1.2f, eyeH * style.pupilHeightRatio);
    const float pupilOffsetX = profile.pupilFocusX * style.pupilFocusXScale;
    const float pupilOffsetY = profile.pupilFocusY * std::max(0.6f, eyeH * style.pupilFocusYScale);
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
    scene.leftPupilRect = Gdiplus::RectF(
        scene.leftEyeRect.X + (scene.leftEyeRect.Width - style.pupilWidthPx) * 0.5f + pupilOffsetX,
        scene.leftEyeRect.Y + (scene.leftEyeRect.Height - pupilH) * 0.5f + pupilOffsetY,
        style.pupilWidthPx,
        pupilH);
    scene.rightPupilRect = Gdiplus::RectF(
        scene.rightEyeRect.X + (scene.rightEyeRect.Width - style.pupilWidthPx) * 0.5f + pupilOffsetX,
        scene.rightEyeRect.Y + (scene.rightEyeRect.Height - pupilH) * 0.5f + pupilOffsetY,
        style.pupilWidthPx,
        pupilH);
    scene.leftEyeHighlightRect = Gdiplus::RectF(
        scene.leftEyeRect.X + style.eyeHighlightInsetXPx,
        scene.leftEyeRect.Y + style.eyeHighlightInsetYPx,
        style.eyeHighlightSizePx,
        style.eyeHighlightSizePx);
    scene.rightEyeHighlightRect = Gdiplus::RectF(
        scene.rightEyeRect.X + style.eyeHighlightInsetXPx,
        scene.rightEyeRect.Y + style.eyeHighlightInsetYPx,
        style.eyeHighlightSizePx,
        style.eyeHighlightSizePx);
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
    scene.leftCheekContourRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.cheekContourXRatio,
        scene.headRect.Y + scene.headRect.Height * style.cheekContourYRatio,
        scene.headRect.Width * style.cheekContourWidthRatio,
        scene.headRect.Height * style.cheekContourHeightRatio);
    scene.rightCheekContourRect = Gdiplus::RectF(
        scene.centerX + scene.headRect.Width * style.cheekContourXRatio - scene.headRect.Width * style.cheekContourWidthRatio,
        scene.headRect.Y + scene.headRect.Height * style.cheekContourYRatio,
        scene.headRect.Width * style.cheekContourWidthRatio,
        scene.headRect.Height * style.cheekContourHeightRatio);
    scene.jawContourRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.jawContourWidthRatio * 0.5f,
        scene.headRect.Y + scene.headRect.Height * style.jawContourYRatio,
        scene.headRect.Width * style.jawContourWidthRatio,
        scene.headRect.Height * style.jawContourHeightRatio);
    scene.muzzlePadRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.muzzlePadWidthRatio * 0.5f,
        scene.headRect.Y + scene.headRect.Height * style.muzzlePadYRatio,
        scene.headRect.Width * style.muzzlePadWidthRatio,
        scene.headRect.Height * style.muzzlePadHeightRatio);
    scene.foreheadPadRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.foreheadPadWidthRatio * 0.5f,
        scene.headRect.Y + scene.headRect.Height * style.foreheadPadYRatio,
        scene.headRect.Width * style.foreheadPadWidthRatio,
        scene.headRect.Height * style.foreheadPadHeightRatio);
    scene.leftTempleContourRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.templeContourXRatio,
        scene.headRect.Y + scene.headRect.Height * style.templeContourYRatio,
        scene.headRect.Width * style.templeContourWidthRatio,
        scene.headRect.Height * style.templeContourHeightRatio);
    scene.rightTempleContourRect = Gdiplus::RectF(
        scene.centerX + scene.headRect.Width * style.templeContourXRatio - scene.headRect.Width * style.templeContourWidthRatio,
        scene.headRect.Y + scene.headRect.Height * style.templeContourYRatio,
        scene.headRect.Width * style.templeContourWidthRatio,
        scene.headRect.Height * style.templeContourHeightRatio);
    scene.leftUnderEyeContourRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.underEyeContourXRatio,
        scene.headRect.Y + scene.headRect.Height * style.underEyeContourYRatio,
        scene.headRect.Width * style.underEyeContourWidthRatio,
        scene.headRect.Height * style.underEyeContourHeightRatio);
    scene.rightUnderEyeContourRect = Gdiplus::RectF(
        scene.centerX + scene.headRect.Width * style.underEyeContourXRatio - scene.headRect.Width * style.underEyeContourWidthRatio,
        scene.headRect.Y + scene.headRect.Height * style.underEyeContourYRatio,
        scene.headRect.Width * style.underEyeContourWidthRatio,
        scene.headRect.Height * style.underEyeContourHeightRatio);
    scene.noseBridgeRect = Gdiplus::RectF(
        scene.centerX - scene.headRect.Width * style.noseBridgeWidthRatio * 0.5f,
        scene.headRect.Y + scene.headRect.Height * style.noseBridgeYRatio,
        scene.headRect.Width * style.noseBridgeWidthRatio,
        scene.headRect.Height * style.noseBridgeHeightRatio);
    scene.eyeHighlightAlpha = profile.eyeHighlightAlpha;
    scene.whiskerStrokeWidth = 1.0f + profile.whiskerSpread * 0.35f;

    const float whiskerAnchorY = scene.headRect.Y + scene.headRect.Height * style.whiskerAnchorYRatio;
    const float leftWhiskerX = scene.centerX - scene.headRect.Width * style.whiskerInnerXRatio;
    const float rightWhiskerX = scene.centerX + scene.headRect.Width * style.whiskerInnerXRatio;
    const float whiskerSpread = profile.whiskerSpread * style.whiskerSpreadScale;
    const float whiskerTilt = profile.whiskerTilt * style.whiskerTiltScale;
    SetWhiskerPair(
        leftWhiskerX,
        whiskerAnchorY,
        -1.0f,
        style.whiskerOuterLengthPx,
        -style.whiskerTopYOffsetPx,
        whiskerSpread,
        -whiskerTilt,
        scene.leftWhiskerStart[0],
        scene.leftWhiskerEnd[0]);
    SetWhiskerPair(
        leftWhiskerX,
        whiskerAnchorY,
        -1.0f,
        style.whiskerMiddleLengthPx,
        0.0f,
        0.0f,
        -whiskerTilt * 0.5f,
        scene.leftWhiskerStart[1],
        scene.leftWhiskerEnd[1]);
    SetWhiskerPair(
        leftWhiskerX,
        whiskerAnchorY,
        -1.0f,
        style.whiskerLowerLengthPx,
        style.whiskerLowerYOffsetPx,
        -whiskerSpread,
        -whiskerTilt,
        scene.leftWhiskerStart[2],
        scene.leftWhiskerEnd[2]);
    SetWhiskerPair(
        rightWhiskerX,
        whiskerAnchorY,
        1.0f,
        style.whiskerOuterLengthPx,
        -style.whiskerTopYOffsetPx,
        whiskerSpread,
        whiskerTilt,
        scene.rightWhiskerStart[0],
        scene.rightWhiskerEnd[0]);
    SetWhiskerPair(
        rightWhiskerX,
        whiskerAnchorY,
        1.0f,
        style.whiskerMiddleLengthPx,
        0.0f,
        0.0f,
        whiskerTilt * 0.5f,
        scene.rightWhiskerStart[1],
        scene.rightWhiskerEnd[1]);
    SetWhiskerPair(
        rightWhiskerX,
        whiskerAnchorY,
        1.0f,
        style.whiskerLowerLengthPx,
        style.whiskerLowerYOffsetPx,
        -whiskerSpread,
        whiskerTilt,
        scene.rightWhiskerStart[2],
        scene.rightWhiskerEnd[2]);
}

} // namespace mousefx::windows
