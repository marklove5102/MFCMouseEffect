#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyAppendagePresenceProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

float Blend(float current, float target, float mix) {
    return current + (target - current) * mix;
}

float ResolveAppendageActionBias(const Win32MouseCompanionRealRendererScene& scene) {
    float bias = 0.0f;
    if (scene.actionOverlay.holdBandVisible) {
        bias += 0.28f;
    }
    if (scene.actionOverlay.dragLineVisible) {
        bias += 0.32f;
    }
    if (scene.actionOverlay.followTrailVisible) {
        bias += 0.22f;
    }
    if (scene.actionOverlay.scrollArcVisible) {
        bias += 0.10f;
    }
    return std::clamp(bias, 0.0f, 0.72f);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyAppendagePresenceProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    scene.previewTailAlphaScale = scene.previewAppendageAlphaScale;
    scene.previewHandAlphaScale = scene.previewAppendageAlphaScale;
    scene.previewLegAlphaScale = scene.previewAppendageAlphaScale;

    if (!scene.modelProxyAppendageLayer.visible) {
        return;
    }

    const float dominance = std::clamp(scene.proxyDominance, 0.0f, 1.0f);
    const float actionBias = ResolveAppendageActionBias(scene);
    const float tailMix = std::clamp(dominance * 0.82f + actionBias * 0.48f, 0.0f, 1.0f);
    const float handMix = std::clamp(dominance * 0.76f + actionBias * 0.56f, 0.0f, 1.0f);
    const float legMix = std::clamp(dominance * 0.78f + actionBias * 0.52f, 0.0f, 1.0f);

    scene.previewTailAlphaScale = Blend(
        scene.previewTailAlphaScale,
        0.18f,
        tailMix);
    scene.previewHandAlphaScale = Blend(
        scene.previewHandAlphaScale,
        0.20f,
        handMix);
    scene.previewLegAlphaScale = Blend(
        scene.previewLegAlphaScale,
        0.22f,
        legMix);

    scene.previewAppendageAlphaScale = std::min(
        scene.previewAppendageAlphaScale,
        std::min(scene.previewTailAlphaScale,
                 std::min(scene.previewHandAlphaScale, scene.previewLegAlphaScale)));
}

} // namespace mousefx::windows
