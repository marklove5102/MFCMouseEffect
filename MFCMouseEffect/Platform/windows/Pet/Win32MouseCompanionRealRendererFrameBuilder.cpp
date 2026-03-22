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
    const float bodyWidthRatio = runtime.hold ? style.holdBodyWidthRatio : style.bodyWidthRatio;
    const float bodyHeightRatio = runtime.hold ? style.holdBodyHeightRatio : style.bodyHeightRatio;
    const float bodyWidthScale = runtime.follow ? style.followBodyWidthScale
        : runtime.click                    ? style.clickBodyWidthScale
        : runtime.drag                     ? style.dragBodyWidthScale
                                           : 1.0f;
    const float bodyHeightScale = runtime.follow ? style.followBodyHeightScale
        : runtime.click                     ? style.clickBodyHeightScale
                                            : 1.0f;
    metrics.bodyWidth = static_cast<float>(width) * bodyWidthRatio * bodyWidthScale;
    metrics.bodyHeight = static_cast<float>(height) * bodyHeightRatio * bodyHeightScale;
    const float headScale = runtime.follow ? style.followHeadScale
        : runtime.hold                    ? style.holdHeadScale
        : runtime.click                   ? style.clickHeadScale
                                          : 1.0f;
    metrics.headWidth = metrics.bodyWidth * style.headWidthRatio * headScale;
    metrics.headHeight = metrics.bodyHeight * style.headHeightRatio * headScale;

    const float glowBoost = std::max({profile.actionIntensity, profile.reactiveIntensity, profile.scrollIntensity});
    const float facingOffset = runtime.facingSign *
        std::min(runtime.facingMomentumPx * style.facingMomentumScale, style.facingMomentumClampPx);
    const float glowStateScale = runtime.follow ? style.followGlowScale
        : runtime.hold                        ? style.holdGlowScale
        : runtime.click                       ? style.clickGlowScale
        : runtime.drag                        ? style.dragGlowScale
                                              : 1.0f;
    const float glowAlphaBias = runtime.follow ? style.followGlowAlphaBias
        : runtime.hold                        ? style.holdGlowAlphaBias
        : runtime.click                       ? style.clickGlowAlphaBias
        : runtime.scroll                      ? style.scrollGlowAlphaBias
        : runtime.drag                        ? style.dragGlowAlphaBias
                                              : 0.0f;
    const float glowStateXOffset = runtime.drag ? style.dragGlowXOffsetPx * runtime.facingSign
        : runtime.scroll                        ? style.scrollGlowXOffsetPx * runtime.facingSign
                                                : 0.0f;
    const float glowStateYOffset = runtime.follow ? style.followGlowYOffsetPx
        : runtime.hold                           ? style.holdGlowYOffsetPx
        : runtime.click                          ? style.clickGlowYOffsetPx
                                                 : 0.0f;
    const float glowScale = (1.0f + glowBoost * style.glowActionScale) * glowStateScale;
    const float shadowStateScale = runtime.follow ? style.followShadowScale
        : runtime.hold                          ? style.holdShadowScale
        : runtime.scroll                        ? style.scrollShadowScale
        : runtime.drag                          ? style.dragShadowScale
                                                : 1.0f;
    const float pedestalStateScale = runtime.follow ? style.followPedestalScale
        : runtime.hold                            ? style.holdPedestalScale
        : runtime.scroll                          ? style.scrollPedestalScale
                                                  : 1.0f;
    const float shadowStateXOffset = runtime.follow ? style.followShadowXOffsetPx
        : runtime.scroll                           ? style.scrollShadowXOffsetPx
        : runtime.drag                             ? style.dragShadowXOffsetPx
                                                   : 0.0f;
    const float shadowStateYOffset = runtime.hold ? style.holdShadowYOffsetPx : 0.0f;
    const float pedestalStateXOffset = runtime.follow ? style.followPedestalXOffsetPx
        : runtime.scroll                             ? style.scrollPedestalXOffsetPx
        : runtime.drag                               ? style.dragPedestalXOffsetPx
                                                     : 0.0f;
    const float pedestalStateYOffset = runtime.hold ? style.holdPedestalYOffsetPx : 0.0f;

    scene.centerX = static_cast<float>(width) * style.centerXRatio + facingOffset + profile.bodyForward + profile.idleHeadSway;
    scene.centerY = static_cast<float>(height) * (runtime.hold ? style.holdCenterYRatio : style.idleCenterYRatio) -
        (runtime.follow ? static_cast<float>(height) * style.followCenterYOffsetRatio : 0.0f) -
        (runtime.drag ? static_cast<float>(height) * style.dragCenterYOffsetRatio : 0.0f) -
        profile.stateLift - profile.breathLift;
    scene.facingSign = runtime.facingSign;
    scene.bodyTiltDeg = profile.scrollLean + profile.dragLean;
    scene.glowAlpha =
        style.glowBaseAlpha + glowBoost * style.glowActionAlphaScale + runtime.headTintAmount * style.glowHeadTintAlphaScale +
        glowAlphaBias;
    scene.bodyStrokeWidth = runtime.click ? (1.9f + profile.actionIntensity * 1.0f)
        : runtime.hold      ? (1.8f + profile.actionIntensity * 0.9f)
        : runtime.scroll    ? (1.8f + profile.scrollIntensity * 0.8f)
                            : 1.8f;
    scene.headStrokeWidth = runtime.drag ? (1.9f + profile.actionIntensity * 0.9f)
        : runtime.click                  ? (1.8f + profile.actionIntensity * 0.8f)
        : runtime.follow                 ? (1.8f + profile.actionIntensity * 0.6f)
                                         : 1.8f;
    scene.limbStrokeWidth = runtime.follow ? (1.2f + profile.actionIntensity * 0.7f)
        : runtime.hold                     ? (1.2f + profile.actionIntensity * 0.5f)
                                           : 1.2f;
    scene.tailStrokeWidth = runtime.follow ? (1.2f + profile.actionIntensity * 0.6f)
        : runtime.scroll                   ? (1.2f + profile.scrollIntensity * 0.5f)
                                           : 1.2f;
    scene.chestStrokeWidth = runtime.hold ? (1.1f + profile.actionIntensity * 0.5f)
        : runtime.click                   ? (1.1f + profile.actionIntensity * 0.4f)
                                          : 1.1f;
    scene.chestFillAlpha = runtime.hold ? (255.0f - profile.actionIntensity * 18.0f)
        : runtime.follow                 ? (248.0f - profile.actionIntensity * 10.0f)
                                         : 255.0f;

    scene.glowRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.glowXOffsetRatio + glowStateXOffset,
        scene.centerY - metrics.bodyHeight * style.glowYOffsetRatio - profile.stateLift * style.glowStateLiftScale +
            glowStateYOffset,
        metrics.bodyWidth * style.glowWidthScale * glowScale,
        metrics.bodyHeight * style.glowHeightScale * glowScale);
    scene.shadowRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.shadowXOffsetRatio * profile.shadowScale * shadowStateScale +
            shadowStateXOffset * runtime.facingSign,
        scene.centerY + metrics.bodyHeight * style.shadowYRatio + shadowStateYOffset,
        metrics.bodyWidth * style.shadowWidthScale * profile.shadowScale * shadowStateScale *
            (1.0f - (profile.breathScale - 1.0f) * style.shadowBreathScale),
        metrics.bodyHeight * style.shadowHeightScale * shadowStateScale);
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
    scene.neckBridgeRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.neckBridgeXOffsetRatio + profile.bodyForward * 0.08f,
        scene.headRect.GetBottom() - metrics.bodyHeight * style.neckBridgeYOffsetRatio,
        metrics.bodyWidth * style.neckBridgeWidthRatio,
        metrics.bodyHeight * style.neckBridgeHeightRatio);
    scene.leftShoulderPatchRect = Gdiplus::RectF(
        scene.bodyRect.X + metrics.bodyWidth * style.shoulderPatchXOffsetRatio,
        scene.bodyRect.Y + metrics.bodyHeight * style.shoulderPatchYOffsetRatio,
        metrics.bodyWidth * style.shoulderPatchWidthRatio,
        metrics.bodyHeight * style.shoulderPatchHeightRatio);
    scene.rightShoulderPatchRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - metrics.bodyWidth * (style.shoulderPatchXOffsetRatio + style.shoulderPatchWidthRatio),
        scene.bodyRect.Y + metrics.bodyHeight * style.shoulderPatchYOffsetRatio,
        metrics.bodyWidth * style.shoulderPatchWidthRatio,
        metrics.bodyHeight * style.shoulderPatchHeightRatio);
    scene.leftHipPatchRect = Gdiplus::RectF(
        scene.bodyRect.X + metrics.bodyWidth * style.hipPatchXOffsetRatio,
        scene.bodyRect.GetBottom() - metrics.bodyHeight * (style.hipPatchYOffsetRatio + style.hipPatchHeightRatio),
        metrics.bodyWidth * style.hipPatchWidthRatio,
        metrics.bodyHeight * style.hipPatchHeightRatio);
    scene.rightHipPatchRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - metrics.bodyWidth * (style.hipPatchXOffsetRatio + style.hipPatchWidthRatio),
        scene.bodyRect.GetBottom() - metrics.bodyHeight * (style.hipPatchYOffsetRatio + style.hipPatchHeightRatio),
        metrics.bodyWidth * style.hipPatchWidthRatio,
        metrics.bodyHeight * style.hipPatchHeightRatio);
    scene.bellyContourRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.bellyContourWidthRatio * 0.5f,
        scene.bodyRect.Y + metrics.bodyHeight * style.bellyContourYRatio,
        metrics.bodyWidth * style.bellyContourWidthRatio,
        metrics.bodyHeight * style.bellyContourHeightRatio);
    scene.sternumContourRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.sternumContourWidthRatio * 0.5f,
        scene.bodyRect.Y + metrics.bodyHeight * style.sternumContourYRatio,
        metrics.bodyWidth * style.sternumContourWidthRatio,
        metrics.bodyHeight * style.sternumContourHeightRatio);
    scene.upperTorsoContourRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.upperTorsoContourWidthRatio * 0.5f,
        scene.bodyRect.Y + metrics.bodyHeight * style.upperTorsoContourYRatio,
        metrics.bodyWidth * style.upperTorsoContourWidthRatio,
        metrics.bodyHeight * style.upperTorsoContourHeightRatio);
    scene.leftBackContourRect = Gdiplus::RectF(
        scene.bodyRect.X + metrics.bodyWidth * style.backContourXRatio,
        scene.bodyRect.Y + metrics.bodyHeight * style.backContourYRatio,
        metrics.bodyWidth * style.backContourWidthRatio,
        metrics.bodyHeight * style.backContourHeightRatio);
    scene.rightBackContourRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - metrics.bodyWidth * (style.backContourXRatio + style.backContourWidthRatio),
        scene.bodyRect.Y + metrics.bodyHeight * style.backContourYRatio,
        metrics.bodyWidth * style.backContourWidthRatio,
        metrics.bodyHeight * style.backContourHeightRatio);
    scene.leftFlankContourRect = Gdiplus::RectF(
        scene.bodyRect.X + metrics.bodyWidth * style.flankContourXRatio,
        scene.bodyRect.Y + metrics.bodyHeight * style.flankContourYRatio,
        metrics.bodyWidth * style.flankContourWidthRatio,
        metrics.bodyHeight * style.flankContourHeightRatio);
    scene.rightFlankContourRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - metrics.bodyWidth * (style.flankContourXRatio + style.flankContourWidthRatio),
        scene.bodyRect.Y + metrics.bodyHeight * style.flankContourYRatio,
        metrics.bodyWidth * style.flankContourWidthRatio,
        metrics.bodyHeight * style.flankContourHeightRatio);
    scene.pedestalRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.pedestalXOffsetRatio * pedestalStateScale +
            pedestalStateXOffset * runtime.facingSign,
        scene.shadowRect.GetBottom() - metrics.bodyHeight * style.pedestalYOffsetRatio + pedestalStateYOffset,
        metrics.bodyWidth * style.pedestalWidthScale * pedestalStateScale,
        metrics.bodyHeight * style.pedestalHeightScale * pedestalStateScale);

    return metrics;
}

} // namespace mousefx::windows
