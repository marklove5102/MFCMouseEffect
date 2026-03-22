#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererFrameBuilder.h"

#include <algorithm>

namespace mousefx::windows {

Win32MouseCompanionRealRendererLayoutMetrics BuildWin32MouseCompanionRealRendererFrame(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    const Win32MouseCompanionRealRendererMotionProfile& profile,
    const Win32MouseCompanionRealRendererStyleProfile& style,
    int width,
    int height,
    Win32MouseCompanionRealRendererScene& scene) {
    Win32MouseCompanionRealRendererLayoutMetrics metrics{};
    metrics.bodyWidth = static_cast<float>(width) * (runtime.hold ? style.holdBodyWidthRatio : style.bodyWidthRatio);
    metrics.bodyHeight = static_cast<float>(height) * (runtime.hold ? style.holdBodyHeightRatio : style.bodyHeightRatio);
    metrics.headWidth = metrics.bodyWidth * style.headWidthRatio;
    metrics.headHeight = metrics.bodyHeight * style.headHeightRatio;

    const float glowBoost = std::max({profile.actionIntensity, profile.reactiveIntensity, profile.scrollIntensity});
    const float facingOffset = runtime.facingSign *
        std::min(runtime.facingMomentumPx * style.facingMomentumScale, style.facingMomentumClampPx);

    scene.centerX = static_cast<float>(width) * style.centerXRatio + facingOffset + profile.bodyForward + profile.idleHeadSway;
    scene.centerY = static_cast<float>(height) * (runtime.hold ? style.holdCenterYRatio : style.idleCenterYRatio) -
        profile.stateLift - profile.breathLift;
    scene.facingSign = runtime.facingSign;
    scene.bodyTiltDeg = profile.scrollLean + profile.dragLean;
    scene.glowAlpha =
        style.glowBaseAlpha + glowBoost * style.glowActionAlphaScale + runtime.headTintAmount * style.glowHeadTintAlphaScale;

    scene.glowRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.glowXOffsetRatio,
        scene.centerY - metrics.bodyHeight * style.glowYOffsetRatio - profile.stateLift * style.glowStateLiftScale,
        metrics.bodyWidth * style.glowWidthScale,
        metrics.bodyHeight * style.glowHeightScale);
    scene.shadowRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.shadowXOffsetRatio * profile.shadowScale,
        scene.centerY + metrics.bodyHeight * style.shadowYRatio,
        metrics.bodyWidth * style.shadowWidthScale * profile.shadowScale *
            (1.0f - (profile.breathScale - 1.0f) * style.shadowBreathScale),
        metrics.bodyHeight * style.shadowHeightScale);
    scene.bodyRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.bodyXOffsetRatio,
        scene.centerY - metrics.bodyHeight * style.bodyYOffsetRatio + profile.clickSquash * style.bodyClickSquashLiftPx,
        metrics.bodyWidth * (1.0f + profile.clickSquash * style.bodyClickSquashWidthScale) * profile.breathScale,
        metrics.bodyHeight * (1.0f - profile.clickSquash * style.bodyClickSquashHeightScale) * profile.breathScale);
    scene.chestRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.chestXOffsetRatio,
        scene.centerY - metrics.bodyHeight * style.chestYOffsetRatio,
        metrics.bodyWidth * style.chestWidthRatio,
        metrics.bodyHeight * style.chestHeightRatio);
    scene.headRect = Gdiplus::RectF(
        scene.centerX - metrics.headWidth * style.headXOffsetRatio + profile.dragLean * style.headDragLeanScale +
            profile.bodyForward * style.headBodyForwardScale,
        scene.bodyRect.Y - metrics.headHeight * style.headYOffsetRatio - profile.actionIntensity * style.headActionLiftPx +
            profile.headNod,
        metrics.headWidth,
        metrics.headHeight);
    scene.pedestalRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.pedestalXOffsetRatio,
        scene.shadowRect.GetBottom() - metrics.bodyHeight * style.pedestalYOffsetRatio,
        metrics.bodyWidth * style.pedestalWidthScale,
        metrics.bodyHeight * style.pedestalHeightScale);

    return metrics;
}

} // namespace mousefx::windows
