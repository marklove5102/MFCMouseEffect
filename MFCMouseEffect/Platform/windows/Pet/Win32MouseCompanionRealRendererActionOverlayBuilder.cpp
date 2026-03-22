#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererActionOverlayBuilder.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

Gdiplus::RectF MakeCenteredRect(float centerX, float centerY, float width, float height) {
    return Gdiplus::RectF(centerX - width * 0.5f, centerY - height * 0.5f, width, height);
}

} // namespace

void BuildWin32MouseCompanionRealRendererActionOverlay(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    const Win32MouseCompanionRealRendererMotionProfile& profile,
    const Win32MouseCompanionRealRendererStyleProfile& style,
    const Win32MouseCompanionRealRendererLayoutMetrics& metrics,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.actionOverlay.accentColor = profile.overlayAccentColor;

    if (runtime.click) {
        const float ringSize = std::max(scene.headRect.Width, scene.headRect.Height) *
            (style.clickRingScale + profile.actionIntensity * style.clickRingIntensityScale);
        scene.actionOverlay.clickRingVisible = true;
        scene.actionOverlay.clickRingRect = MakeCenteredRect(
            scene.centerX,
            scene.headRect.Y + scene.headRect.Height * style.clickRingCenterYRatio,
            ringSize,
            ringSize * style.clickRingHeightScale);
    }

    if (runtime.hold) {
        scene.actionOverlay.holdBandVisible = true;
        scene.actionOverlay.holdBandRect = Gdiplus::RectF(
            scene.leftHandRect.X + scene.leftHandRect.Width * style.holdBandInsetRatio,
            std::min(scene.leftHandRect.Y, scene.rightHandRect.Y) + metrics.bodyHeight * style.holdBandYOffsetRatio,
            (scene.rightHandRect.GetRight() - scene.leftHandRect.X) - scene.leftHandRect.Width * style.holdBandWidthInsetRatio,
            metrics.bodyHeight * style.holdBandHeightRatio);
    }

    if (runtime.scroll) {
        scene.actionOverlay.scrollArcVisible = true;
        scene.actionOverlay.scrollArcRect = MakeCenteredRect(
            scene.centerX,
            scene.centerY - metrics.bodyHeight * style.scrollArcCenterYRatio,
            metrics.bodyWidth * style.scrollArcWidthRatio,
            metrics.bodyHeight * style.scrollArcHeightRatio);
        scene.actionOverlay.scrollArcStartDeg = runtime.scrollSignedIntensity >= 0.0f ? 204.0f : 24.0f;
        scene.actionOverlay.scrollArcSweepDeg = runtime.scrollSignedIntensity >= 0.0f ? 148.0f : -148.0f;
    }

    if (runtime.drag) {
        scene.actionOverlay.dragLineVisible = true;
        scene.actionOverlay.dragLineStart = Gdiplus::PointF(
            scene.centerX - runtime.facingSign * metrics.bodyWidth * style.dragLineStartXRatio,
            scene.centerY - metrics.bodyHeight * style.dragLineStartYRatio);
        scene.actionOverlay.dragLineEnd = Gdiplus::PointF(
            scene.centerX + runtime.facingSign * metrics.bodyWidth * style.dragLineEndXRatio,
            scene.centerY - metrics.bodyHeight * style.dragLineEndYRatio);
    }

    if (runtime.follow) {
        scene.actionOverlay.followTrailVisible = true;
        const float trailBaseX = scene.centerX - runtime.facingSign * metrics.bodyWidth * style.followTrailBaseXRatio;
        const float trailBaseY = scene.centerY + metrics.bodyHeight * style.followTrailBaseYRatio;
        for (size_t i = 0; i < scene.actionOverlay.followTrailRects.size(); ++i) {
            const float scale = 1.0f - static_cast<float>(i) * style.followTrailShrinkPerStep;
            scene.actionOverlay.followTrailRects[i] = MakeCenteredRect(
                trailBaseX - runtime.facingSign * metrics.bodyWidth * style.followTrailStepXRatio * static_cast<float>(i),
                trailBaseY + metrics.bodyHeight * style.followTrailStepYRatio * static_cast<float>(i),
                metrics.bodyWidth * style.followTrailWidthRatio * scale,
                metrics.bodyHeight * style.followTrailHeightRatio * scale);
        }
    }
}

} // namespace mousefx::windows
