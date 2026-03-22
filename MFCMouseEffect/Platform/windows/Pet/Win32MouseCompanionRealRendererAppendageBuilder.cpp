#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAppendageBuilder.h"

namespace mousefx::windows {

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

    scene.tailRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - metrics.bodyWidth * style.tailXOffsetRatio -
            scene.facingSign * metrics.bodyWidth * style.tailFacingRatio +
            profile.idleTailSway * scene.facingSign + profile.tailSwing,
        scene.centerY - metrics.bodyHeight * style.tailYOffsetRatio + profile.tailLift,
        metrics.bodyWidth * style.tailWidthRatio,
        metrics.bodyHeight * style.tailHeightRatio);

    const float earBaseY = scene.headRect.Y + scene.headRect.Height * style.earBaseYOffsetRatio;
    const float earTipY = scene.headRect.Y - scene.headRect.Height * style.earTipYOffsetRatio - profile.earLift - poseEarLift -
        profile.idleEarCadence;
    const float rightEarTipY = scene.headRect.Y - scene.headRect.Height * style.earTipYOffsetRatio - profile.earLift -
        poseRightEarLift + profile.idleEarCadence * style.earIdleCadenceRatio;
    const float earBaseOffset = scene.headRect.Width * style.earBaseOffsetRatio;
    scene.leftEar = {{
        Gdiplus::PointF(scene.centerX - earBaseOffset, earBaseY),
        Gdiplus::PointF(
            scene.centerX - earBaseOffset - scene.headRect.Width * style.earOuterSpreadRatio + poseEarSpread - profile.earSwing -
                profile.earSpreadPulse -
                profile.idleHeadSway,
            scene.headRect.Y + scene.headRect.Height * style.earBaseYRatio),
        Gdiplus::PointF(
            scene.centerX - scene.headRect.Width * style.earTipSpreadRatio + poseEarSpread -
                profile.earSwing * style.leftEarSwingScale - profile.earSpreadPulse -
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
                profile.earSwing + profile.earSpreadPulse + profile.idleHeadSway,
            scene.headRect.Y + scene.headRect.Height * style.earBaseYRatio),
        Gdiplus::PointF(
            scene.centerX + scene.headRect.Width * style.earTipSpreadRatio + poseRightEarSpread -
                profile.earSwing * style.rightEarSwingScale + profile.earSpreadPulse +
                profile.idleHeadSway,
            rightEarTipY),
        Gdiplus::PointF(
            scene.centerX + scene.headRect.Width * style.earRootInsetRatio,
            scene.headRect.Y + scene.headRect.Height * style.earRootYRatio),
    }};

    scene.leftHandRect = Gdiplus::RectF(
        scene.bodyRect.X - metrics.bodyWidth * style.handXOffsetRatio - profile.handSwing,
        scene.centerY - metrics.bodyHeight * style.handYOffsetRatio - profile.handLift - poseLeftHandLift + profile.headNod * style.handHeadNodRatio -
            profile.idleHandFloat,
        metrics.bodyWidth * style.handWidthRatio,
        metrics.bodyHeight * style.handHeightRatio);
    scene.rightHandRect = Gdiplus::RectF(
        scene.bodyRect.GetRight() - metrics.bodyWidth * style.tailXOffsetRatio + profile.handSwing,
        scene.centerY - metrics.bodyHeight * style.handYOffsetRatio - profile.handLift - poseRightHandLift + profile.headNod * style.handHeadNodRatio +
            profile.idleHandFloat,
        metrics.bodyWidth * style.handWidthRatio,
        metrics.bodyHeight * style.handHeightRatio);
    scene.leftLegRect = Gdiplus::RectF(
        scene.centerX - metrics.bodyWidth * style.legLeftXRatio - profile.legStride * style.legStrideRatio + poseLeftLegShift,
        scene.bodyRect.GetBottom() - metrics.bodyHeight * style.legYOffsetRatio + (runtime.follow ? style.followLegPhasePx : 0.0f) -
            profile.legLift,
        metrics.bodyWidth * style.legWidthRatio,
        metrics.bodyHeight * style.legHeightRatio);
    scene.rightLegRect = Gdiplus::RectF(
        scene.centerX + metrics.bodyWidth * style.legRightXRatio + profile.legStride * style.legStrideRatio + poseRightLegShift,
        scene.bodyRect.GetBottom() - metrics.bodyHeight * style.legYOffsetRatio - (runtime.follow ? style.followLegPhasePx : 0.0f) +
            profile.legLift * 0.45f,
        metrics.bodyWidth * style.legWidthRatio,
        metrics.bodyHeight * style.legHeightRatio);
}

} // namespace mousefx::windows
