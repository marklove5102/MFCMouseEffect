#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAppendageBuilder.h"

namespace mousefx::windows {

namespace {

Gdiplus::RectF BuildPadRect(
    const Gdiplus::RectF& limbRect,
    const Win32MouseCompanionRealRendererStyleProfile& style) {
    const float padWidth = limbRect.Width * style.pawPadWidthRatio;
    const float padHeight = limbRect.Height * style.pawPadHeightRatio;
    return Gdiplus::RectF(
        limbRect.X + (limbRect.Width - padWidth) * 0.5f,
        limbRect.GetBottom() - padHeight - limbRect.Height * style.pawPadBottomInsetRatio,
        padWidth,
        padHeight);
}

} // namespace

void BuildWin32MouseCompanionRealRendererAppendages(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    const Win32MouseCompanionRealRendererMotionProfile& profile,
    const Win32MouseCompanionRealRendererStyleProfile& style,
    const Win32MouseCompanionRealRendererLayoutMetrics& metrics,
    Win32MouseCompanionRealRendererScene& scene) {
    const float poseEarLift = runtime.leftEarPose ? runtime.leftEarPose->position[1] * style.leftEarPoseLiftScale : 0.0f;
    const float poseEarSpread = runtime.leftEarPose ? -runtime.leftEarPose->position[0] * style.leftEarPoseSpreadScale : 0.0f;
    const float poseRightEarLift = runtime.rightEarPose ? runtime.rightEarPose->position[1] * style.rightEarPoseLiftScale : 0.0f;
    const float poseRightEarSpread = runtime.rightEarPose ? runtime.rightEarPose->position[0] * style.rightEarPoseSpreadScale : 0.0f;
    const float poseLeftHandLift = runtime.leftHandPose ? -runtime.leftHandPose->position[1] * style.leftHandPoseLiftScale : 0.0f;
    const float poseRightHandLift = runtime.rightHandPose ? -runtime.rightHandPose->position[1] * style.rightHandPoseLiftScale : 0.0f;
    const float poseLeftLegShift = runtime.leftLegPose ? runtime.leftLegPose->position[0] * style.leftLegPoseShiftScale : 0.0f;
    const float poseRightLegShift = runtime.rightLegPose ? runtime.rightLegPose->position[0] * style.rightLegPoseShiftScale : 0.0f;
    const float handReachBias = runtime.follow ? metrics.bodyWidth * style.followHandReachRatio
        : runtime.hold                        ? -metrics.bodyWidth * style.holdHandTuckRatio
        : runtime.drag                        ? metrics.bodyWidth * style.dragHandReachRatio
                                              : 0.0f;
    const float legStanceBias = runtime.follow ? metrics.bodyWidth * style.followLegStanceRatio
        : runtime.hold                        ? -metrics.bodyWidth * style.holdLegStanceRatio
                                              : 0.0f;
    const float tailWidthScale = runtime.follow ? style.followTailWidthScale
        : runtime.hold                         ? style.holdTailWidthScale
                                               : 1.0f;
    const float tailHeightScale = runtime.scroll ? style.scrollTailHeightScale : 1.0f;
    const float earSpreadScale = runtime.follow ? style.followEarSpreadScale
        : runtime.hold                         ? style.holdEarSpreadScale
                                               : 1.0f;
    const float clickEarLift = runtime.click ? style.clickEarLiftPx : 0.0f;

    scene.tailRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - metrics.bodyWidth * style.tailXOffsetRatio -
            scene.facingSign * metrics.bodyWidth * style.tailFacingRatio +
            profile.idleTailSway * scene.facingSign + profile.tailSwing,
        scene.centerY - metrics.bodyHeight * style.tailYOffsetRatio + profile.tailLift,
        metrics.bodyWidth * style.tailWidthRatio * tailWidthScale,
        metrics.bodyHeight * style.tailHeightRatio * tailHeightScale);
    scene.tailRootCuffRect = Gdiplus::RectF(
        scene.tailRect.X - scene.tailRect.Width * style.tailRootCuffXOffsetRatio,
        scene.tailRect.Y + scene.tailRect.Height * style.tailRootCuffYOffsetRatio,
        scene.tailRect.Width * style.tailRootCuffWidthRatio,
        scene.tailRect.Height * style.tailRootCuffHeightRatio);
    scene.tailTipRect = Gdiplus::RectF(
        scene.tailRect.GetRight() - scene.tailRect.Width * (style.tailTipXOffsetRatio + style.tailTipWidthRatio),
        scene.tailRect.Y + scene.tailRect.Height * style.tailTipYOffsetRatio - profile.tailLift * 0.08f,
        scene.tailRect.Width * style.tailTipWidthRatio,
        scene.tailRect.Height * style.tailTipHeightRatio);

    const float earBaseY = scene.headRect.Y + scene.headRect.Height * style.earBaseYOffsetRatio;
    const float earTipY = scene.headRect.Y - scene.headRect.Height * style.earTipYOffsetRatio - profile.earLift - poseEarLift -
        profile.idleEarCadence - clickEarLift;
    const float rightEarTipY = scene.headRect.Y - scene.headRect.Height * style.earTipYOffsetRatio - profile.earLift -
        poseRightEarLift + profile.idleEarCadence * style.earIdleCadenceRatio - clickEarLift;
    const float earBaseOffset = scene.headRect.Width * style.earBaseOffsetRatio;
    const float earRootCuffWidth = scene.headRect.Width * style.earRootCuffWidthRatio;
    const float earRootCuffHeight = scene.headRect.Height * style.earRootCuffHeightRatio;
    scene.leftEarRootCuffRect = Gdiplus::RectF(
        scene.centerX - earBaseOffset - earRootCuffWidth * 0.55f,
        earBaseY - earRootCuffHeight * (0.5f + style.earRootCuffYOffsetRatio),
        earRootCuffWidth,
        earRootCuffHeight);
    scene.rightEarRootCuffRect = Gdiplus::RectF(
        scene.centerX + earBaseOffset - earRootCuffWidth * 0.45f,
        earBaseY - earRootCuffHeight * (0.5f + style.earRootCuffYOffsetRatio),
        earRootCuffWidth,
        earRootCuffHeight);
    scene.leftEar = {{
        Gdiplus::PointF(scene.centerX - earBaseOffset, earBaseY),
        Gdiplus::PointF(
            scene.centerX - earBaseOffset - scene.headRect.Width * style.earOuterSpreadRatio + poseEarSpread - profile.earSwing -
                profile.earSpreadPulse * earSpreadScale -
                profile.idleHeadSway,
            scene.headRect.Y + scene.headRect.Height * style.earBaseYRatio),
        Gdiplus::PointF(
            scene.centerX - scene.headRect.Width * style.earTipSpreadRatio * earSpreadScale + poseEarSpread -
                profile.earSwing * style.leftEarSwingScale - profile.earSpreadPulse * earSpreadScale -
                profile.idleHeadSway,
            earTipY),
        Gdiplus::PointF(
            scene.centerX - scene.headRect.Width * style.earRootInsetRatio,
            scene.headRect.Y + scene.headRect.Height * style.earRootYRatio),
    }};
    scene.rightEar = {{
        Gdiplus::PointF(scene.centerX + earBaseOffset, earBaseY),
        Gdiplus::PointF(
            scene.centerX + earBaseOffset + scene.headRect.Width * style.earOuterSpreadRatio + poseRightEarSpread -
                profile.earSwing + profile.earSpreadPulse * earSpreadScale + profile.idleHeadSway,
            scene.headRect.Y + scene.headRect.Height * style.earBaseYRatio),
        Gdiplus::PointF(
            scene.centerX + scene.headRect.Width * style.earTipSpreadRatio * earSpreadScale + poseRightEarSpread -
                profile.earSwing * style.rightEarSwingScale + profile.earSpreadPulse * earSpreadScale +
                profile.idleHeadSway,
            rightEarTipY),
        Gdiplus::PointF(
            scene.centerX + scene.headRect.Width * style.earRootInsetRatio,
            scene.headRect.Y + scene.headRect.Height * style.earRootYRatio),
    }};

    scene.leftHandRect = Gdiplus::RectF(
        scene.bodyRect.X - metrics.bodyWidth * style.handXOffsetRatio - profile.handSwing - handReachBias,
        scene.centerY - metrics.bodyHeight * style.handYOffsetRatio - profile.handLift - poseLeftHandLift + profile.headNod * style.handHeadNodRatio -
            profile.idleHandFloat,
        metrics.bodyWidth * style.handWidthRatio,
        metrics.bodyHeight * style.handHeightRatio);
    scene.rightHandRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - metrics.bodyWidth * style.tailXOffsetRatio + profile.handSwing + handReachBias,
        scene.centerY - metrics.bodyHeight * style.handYOffsetRatio - profile.handLift - poseRightHandLift + profile.headNod * style.handHeadNodRatio +
            profile.idleHandFloat,
        metrics.bodyWidth * style.handWidthRatio,
        metrics.bodyHeight * style.handHeightRatio);
    scene.leftHandRootCuffRect = Gdiplus::RectF(
        scene.leftHandRect.X + scene.leftHandRect.Width * style.handRootCuffInsetRatio,
        scene.leftHandRect.Y - scene.leftHandRect.Height * style.handRootCuffHeightRatio * 0.35f,
        scene.leftHandRect.Width * style.handRootCuffWidthRatio,
        scene.leftHandRect.Height * style.handRootCuffHeightRatio);
    scene.rightHandRootCuffRect = Gdiplus::RectF(
        scene.rightHandRect.GetRight() - scene.rightHandRect.Width * (style.handRootCuffInsetRatio + style.handRootCuffWidthRatio),
        scene.rightHandRect.Y - scene.rightHandRect.Height * style.handRootCuffHeightRatio * 0.35f,
        scene.rightHandRect.Width * style.handRootCuffWidthRatio,
        scene.rightHandRect.Height * style.handRootCuffHeightRatio);
    scene.leftHandSilhouetteBridgeRect = Gdiplus::RectF(
        scene.leftHandRect.GetRight() - scene.leftHandRect.Width * style.handSilhouetteBridgeWidthRatio,
        scene.leftHandRootCuffRect.Y + scene.leftHandRootCuffRect.Height * 0.15f,
        scene.leftHandRect.Width * style.handSilhouetteBridgeWidthRatio,
        scene.leftHandRect.Height * style.handSilhouetteBridgeHeightRatio);
    scene.rightHandSilhouetteBridgeRect = Gdiplus::RectF(
        scene.rightHandRect.X,
        scene.rightHandRootCuffRect.Y + scene.rightHandRootCuffRect.Height * 0.15f,
        scene.rightHandRect.Width * style.handSilhouetteBridgeWidthRatio,
        scene.rightHandRect.Height * style.handSilhouetteBridgeHeightRatio);
    scene.leftLegRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.legLeftXRatio - profile.legStride * style.legStrideRatio + poseLeftLegShift - legStanceBias,
        scene.bodyRect.GetBottom() - metrics.bodyHeight * style.legYOffsetRatio + (runtime.follow ? style.followLegPhasePx : 0.0f) -
            profile.legLift,
        metrics.bodyWidth * style.legWidthRatio,
        metrics.bodyHeight * style.legHeightRatio);
    scene.rightLegRect = Gdiplus::RectF(
        scene.centerX + metrics.bodyWidth * style.legRightXRatio + profile.legStride * style.legStrideRatio + poseRightLegShift + legStanceBias,
        scene.bodyRect.GetBottom() - metrics.bodyHeight * style.legYOffsetRatio - (runtime.follow ? style.followLegPhasePx : 0.0f) +
            profile.legLift * 0.45f,
        metrics.bodyWidth * style.legWidthRatio,
        metrics.bodyHeight * style.legHeightRatio);
    scene.leftLegRootCuffRect = Gdiplus::RectF(
        scene.leftLegRect.X + scene.leftLegRect.Width * style.legRootCuffInsetRatio,
        scene.leftLegRect.Y - scene.leftLegRect.Height * style.legRootCuffHeightRatio * 0.30f,
        scene.leftLegRect.Width * style.legRootCuffWidthRatio,
        scene.leftLegRect.Height * style.legRootCuffHeightRatio);
    scene.rightLegRootCuffRect = Gdiplus::RectF(
        scene.rightLegRect.GetRight() - scene.rightLegRect.Width * (style.legRootCuffInsetRatio + style.legRootCuffWidthRatio),
        scene.rightLegRect.Y - scene.rightLegRect.Height * style.legRootCuffHeightRatio * 0.30f,
        scene.rightLegRect.Width * style.legRootCuffWidthRatio,
        scene.rightLegRect.Height * style.legRootCuffHeightRatio);
    scene.leftLegSilhouetteBridgeRect = Gdiplus::RectF(
        scene.leftLegRect.GetRight() - scene.leftLegRect.Width * style.legSilhouetteBridgeWidthRatio,
        scene.leftLegRootCuffRect.Y + scene.leftLegRootCuffRect.Height * 0.10f,
        scene.leftLegRect.Width * style.legSilhouetteBridgeWidthRatio,
        scene.leftLegRect.Height * style.legSilhouetteBridgeHeightRatio);
    scene.rightLegSilhouetteBridgeRect = Gdiplus::RectF(
        scene.rightLegRect.X,
        scene.rightLegRootCuffRect.Y + scene.rightLegRootCuffRect.Height * 0.10f,
        scene.rightLegRect.Width * style.legSilhouetteBridgeWidthRatio,
        scene.rightLegRect.Height * style.legSilhouetteBridgeHeightRatio);
    scene.leftHandPadRect = BuildPadRect(scene.leftHandRect, style);
    scene.rightHandPadRect = BuildPadRect(scene.rightHandRect, style);
    scene.leftLegPadRect = BuildPadRect(scene.leftLegRect, style);
    scene.rightLegPadRect = BuildPadRect(scene.rightLegRect, style);
}

} // namespace mousefx::windows
