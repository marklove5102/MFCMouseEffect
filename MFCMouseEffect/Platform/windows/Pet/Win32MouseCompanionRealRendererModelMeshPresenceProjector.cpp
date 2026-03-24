#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelMeshPresenceProjector.h"

#include <algorithm>

namespace mousefx::windows {

void ApplyWin32MouseCompanionRealRendererModelMeshPresenceProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    if (!scene.modelMeshVisible || scene.modelMeshTriangles.empty()) {
        return;
    }

    scene.modelSceneGraphVisible = false;
    scene.modelProxyVisible = false;
    scene.modelProxyFrameLayer.visible = false;
    scene.modelProxyContourLayer.visible = false;
    scene.modelProxyAppendageLayer.visible = false;
    scene.modelProxyDetailLayer.visible = false;

    scene.previewBodyAlphaScale *= 0.12f;
    scene.previewHeadAlphaScale *= 0.10f;
    scene.previewAppendageAlphaScale *= 0.10f;
    scene.previewTailAlphaScale *= 0.08f;
    scene.previewHandAlphaScale *= 0.08f;
    scene.previewLegAlphaScale *= 0.08f;
    scene.previewDetailAlphaScale *= 0.08f;
    scene.previewAdornmentAlphaScale *= 0.12f;
    scene.previewTailStrokeScale *= 0.18f;
    scene.previewHandStrokeScale *= 0.18f;
    scene.previewLegStrokeScale *= 0.18f;
    scene.shadowAlphaScale *= 0.82f;
    scene.pedestalAlphaScale *= 0.80f;
    scene.proxyDominance = std::max(scene.proxyDominance, 0.82f);

    for (auto& triangle : scene.modelMeshTriangles) {
        triangle.alpha = std::clamp(triangle.alpha * 1.24f, 144.0f, 232.0f);
    }
}

} // namespace mousefx::windows
