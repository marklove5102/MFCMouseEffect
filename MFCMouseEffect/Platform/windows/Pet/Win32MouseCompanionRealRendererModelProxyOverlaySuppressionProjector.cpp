#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyOverlaySuppressionProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

float ResolveProxyActionPresence(const Win32MouseCompanionRealRendererScene& scene) {
    float presence = 0.0f;
    if (scene.modelProxyActionLayer.clickShellVisible) {
        presence += 1.00f;
    }
    if (scene.modelProxyActionLayer.holdShellVisible) {
        presence += 0.88f;
    }
    if (scene.modelProxyActionLayer.scrollShellVisible) {
        presence += 0.90f;
    }
    if (scene.modelProxyActionLayer.dragShellVisible) {
        presence += 0.76f;
    }
    if (scene.modelProxyActionLayer.followShellVisible) {
        presence += 0.72f;
    }
    return std::clamp(presence, 0.0f, 2.8f);
}

float ClampAlpha(float value) {
    return std::clamp(value, 0.0f, 255.0f);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyOverlaySuppressionProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    if (!scene.modelProxyVisible) {
        return;
    }

    const float actionPresence = ResolveProxyActionPresence(scene);
    if (actionPresence <= 0.0f) {
        return;
    }

    const float dominanceMix = std::clamp(
        0.36f + scene.proxyDominance * 0.64f + actionPresence * 0.08f,
        0.36f,
        0.92f);
    const float alphaScale = 1.0f - dominanceMix * 0.52f;
    const float strokeScale = 1.0f - dominanceMix * 0.18f;

    scene.actionOverlay.clickRingAlpha = ClampAlpha(scene.actionOverlay.clickRingAlpha * alphaScale);
    scene.actionOverlay.holdBandAlpha = ClampAlpha(scene.actionOverlay.holdBandAlpha * alphaScale);
    scene.actionOverlay.scrollArcAlpha = ClampAlpha(scene.actionOverlay.scrollArcAlpha * alphaScale);
    scene.actionOverlay.dragLineAlpha = ClampAlpha(scene.actionOverlay.dragLineAlpha * alphaScale);
    scene.actionOverlay.followTrailBaseAlpha =
        ClampAlpha(scene.actionOverlay.followTrailBaseAlpha * alphaScale);

    scene.actionOverlay.clickRingStrokeWidth =
        std::max(1.2f, scene.actionOverlay.clickRingStrokeWidth * strokeScale);
    scene.actionOverlay.scrollArcStrokeWidth =
        std::max(1.4f, scene.actionOverlay.scrollArcStrokeWidth * strokeScale);
    scene.actionOverlay.dragLineStrokeWidth =
        std::max(1.2f, scene.actionOverlay.dragLineStrokeWidth * strokeScale);
}

} // namespace mousefx::windows
