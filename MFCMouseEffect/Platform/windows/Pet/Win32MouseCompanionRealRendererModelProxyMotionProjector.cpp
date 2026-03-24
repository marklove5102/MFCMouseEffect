#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyMotionProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

float ResolveProxyActionDrive(const Win32MouseCompanionRealRendererScene& scene) {
    float drive = 0.0f;
    if (scene.modelProxyActionLayer.clickShellVisible) {
        drive += 1.00f;
    }
    if (scene.modelProxyActionLayer.holdShellVisible) {
        drive += 0.86f;
    }
    if (scene.modelProxyActionLayer.scrollShellVisible) {
        drive += 0.92f;
    }
    if (scene.modelProxyActionLayer.dragShellVisible) {
        drive += 0.74f;
    }
    if (scene.modelProxyActionLayer.followShellVisible) {
        drive += 0.70f;
    }
    return std::clamp(drive, 0.0f, 2.8f);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyMotionProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    if (!scene.modelProxyVisible) {
        return;
    }

    const float actionDrive = ResolveProxyActionDrive(scene);
    if (actionDrive <= 0.0f) {
        return;
    }

    const float dominanceMix = 0.52f + scene.proxyDominance * 0.48f;
    const float bodyDrive = actionDrive * dominanceMix;

    scene.bodyTiltDeg += 0.72f * bodyDrive * scene.facingSign;
    scene.headAnchorScale *= 1.0f + 0.012f * bodyDrive;
    scene.appendageAnchorScale *= 1.0f + 0.018f * bodyDrive;
    scene.overlayAnchorScale *= 1.0f + 0.012f * bodyDrive;
    scene.groundingAnchorScale *= 1.0f + 0.010f * bodyDrive;

    scene.glowAlpha = std::clamp(
        scene.glowAlpha + 12.0f * bodyDrive,
        0.0f,
        255.0f);
    scene.eyeHighlightAlpha = std::clamp(
        scene.eyeHighlightAlpha + 8.0f * bodyDrive,
        0.0f,
        255.0f);
    scene.mouthSweepDeg = std::clamp(
        scene.mouthSweepDeg + 4.0f * bodyDrive,
        108.0f,
        200.0f);
    scene.poseBadgeAlpha = std::clamp(
        scene.poseBadgeAlpha + 10.0f * bodyDrive,
        0.0f,
        255.0f);
    scene.accessoryAlphaScale = std::min(
        1.40f,
        scene.accessoryAlphaScale * (1.0f + 0.020f * bodyDrive));
    scene.shadowAlphaScale *= 1.0f + 0.016f * bodyDrive;
    scene.pedestalAlphaScale *= 1.0f + 0.014f * bodyDrive;
}

} // namespace mousefx::windows
