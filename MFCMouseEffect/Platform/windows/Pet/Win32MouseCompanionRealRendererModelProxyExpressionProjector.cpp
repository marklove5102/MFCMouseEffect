#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyExpressionProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

const Win32MouseCompanionRealRendererModelProxySilhouette* FindHeadSilhouette(
    const Win32MouseCompanionRealRendererScene& scene) {
    for (const auto& silhouette : scene.modelProxySilhouettes) {
        if (silhouette.logicalNode == "head") {
            return &silhouette;
        }
    }
    return nullptr;
}

float Blend(float current, float target, float mix) {
    return current + (target - current) * mix;
}

float ClampDegrees(float value, float minValue, float maxValue) {
    return std::clamp(value, minValue, maxValue);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyExpressionProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    const auto* headSilhouette = FindHeadSilhouette(scene);
    if (headSilhouette == nullptr || headSilhouette->bounds.Width <= 0.0f ||
        headSilhouette->bounds.Height <= 0.0f || scene.headRect.Width <= 0.0f ||
        scene.headRect.Height <= 0.0f) {
        return;
    }

    const float widthRatio = headSilhouette->bounds.Width / scene.headRect.Width;
    const float heightRatio = headSilhouette->bounds.Height / scene.headRect.Height;
    const float aspectRatio = widthRatio / std::max(0.01f, heightRatio);
    const float highlightTarget =
        std::clamp(scene.eyeHighlightAlpha * (0.96f + widthRatio * 0.08f), 128.0f, 255.0f);
    const float whiskerTarget =
        std::clamp(scene.whiskerStrokeWidth * (0.94f + heightRatio * 0.10f), 0.85f, 2.2f);
    const float mouthSweepTarget =
        ClampDegrees(scene.mouthSweepDeg * (0.94f + aspectRatio * 0.08f), 118.0f, 188.0f);
    const float mouthStartTarget =
        ClampDegrees(scene.mouthStartDeg + (heightRatio - widthRatio) * 9.0f, -6.0f, 26.0f);

    scene.eyeHighlightAlpha = Blend(scene.eyeHighlightAlpha, highlightTarget, 0.34f);
    scene.whiskerStrokeWidth = Blend(scene.whiskerStrokeWidth, whiskerTarget, 0.30f);
    scene.mouthSweepDeg = Blend(scene.mouthSweepDeg, mouthSweepTarget, 0.32f);
    scene.mouthStartDeg = Blend(scene.mouthStartDeg, mouthStartTarget, 0.32f);
    scene.overlayAnchorScale = std::min(1.26f, scene.overlayAnchorScale * (1.0f + heightRatio * 0.012f));
    scene.groundingAnchorScale = std::min(1.24f, scene.groundingAnchorScale * (1.0f + widthRatio * 0.010f));
}

} // namespace mousefx::windows
