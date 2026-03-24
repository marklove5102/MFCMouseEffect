#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyDetailLayerProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

const Win32MouseCompanionRealRendererModelProxySilhouette* FindSilhouette(
    const Win32MouseCompanionRealRendererScene& scene,
    const char* logicalNode) {
    for (const auto& silhouette : scene.modelProxySilhouettes) {
        if (silhouette.logicalNode == (logicalNode ? logicalNode : "")) {
            return &silhouette;
        }
    }
    return nullptr;
}

float Blend(float current, float target, float mix) {
    return current + (target - current) * mix;
}

Gdiplus::PointF BlendPoint(
    const Gdiplus::PointF& current,
    const Gdiplus::PointF& target,
    float mix) {
    return Gdiplus::PointF(
        Blend(current.X, target.X, mix),
        Blend(current.Y, target.Y, mix));
}

Gdiplus::PointF MapPoint(
    const Gdiplus::RectF& from,
    const Gdiplus::RectF& to,
    const Gdiplus::PointF& point) {
    const float xNorm = from.Width > 0.0f ? (point.X - from.X) / from.Width : 0.5f;
    const float yNorm = from.Height > 0.0f ? (point.Y - from.Y) / from.Height : 0.5f;
    return Gdiplus::PointF(to.X + to.Width * xNorm, to.Y + to.Height * yNorm);
}

Gdiplus::RectF MapRect(
    const Gdiplus::RectF& from,
    const Gdiplus::RectF& to,
    const Gdiplus::RectF& rect) {
    const Gdiplus::PointF topLeft = MapPoint(from, to, Gdiplus::PointF(rect.X, rect.Y));
    const Gdiplus::PointF bottomRight =
        MapPoint(from, to, Gdiplus::PointF(rect.GetRight(), rect.GetBottom()));
    return Gdiplus::RectF(
        topLeft.X,
        topLeft.Y,
        bottomRight.X - topLeft.X,
        bottomRight.Y - topLeft.Y);
}

template <size_t N>
void MapLineArray(
    const std::array<Gdiplus::PointF, N>& start,
    const std::array<Gdiplus::PointF, N>& end,
    const Gdiplus::RectF& from,
    const Gdiplus::RectF& to,
    std::array<Gdiplus::PointF, N>* outStart,
    std::array<Gdiplus::PointF, N>* outEnd,
    float mix) {
    if (outStart == nullptr || outEnd == nullptr) {
        return;
    }
    for (size_t i = 0; i < N; ++i) {
        (*outStart)[i] = BlendPoint(start[i], MapPoint(from, to, start[i]), mix);
        (*outEnd)[i] = BlendPoint(end[i], MapPoint(from, to, end[i]), mix);
    }
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyDetailLayerProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    scene.modelProxyDetailLayer = Win32MouseCompanionRealRendererModelProxyDetailLayer{};

    const auto* headSilhouette = FindSilhouette(scene, "head");
    if (headSilhouette == nullptr || headSilhouette->bounds.Width <= 0.0f ||
        headSilhouette->bounds.Height <= 0.0f) {
        return;
    }

    const Gdiplus::RectF oldHeadRect = scene.headRect;
    const Gdiplus::RectF targetHeadRect(
        headSilhouette->bounds.X,
        headSilhouette->bounds.Y,
        std::max(oldHeadRect.Width * 0.82f, headSilhouette->bounds.Width),
        std::max(oldHeadRect.Height * 0.82f, headSilhouette->bounds.Height));

    scene.modelProxyDetailLayer.visible = true;
    scene.modelProxyDetailLayer.leftEyeRect = MapRect(oldHeadRect, targetHeadRect, scene.leftEyeRect);
    scene.modelProxyDetailLayer.rightEyeRect = MapRect(oldHeadRect, targetHeadRect, scene.rightEyeRect);
    scene.modelProxyDetailLayer.leftPupilRect = MapRect(oldHeadRect, targetHeadRect, scene.leftPupilRect);
    scene.modelProxyDetailLayer.rightPupilRect = MapRect(oldHeadRect, targetHeadRect, scene.rightPupilRect);
    scene.modelProxyDetailLayer.leftHighlightRect =
        MapRect(oldHeadRect, targetHeadRect, scene.leftEyeHighlightRect);
    scene.modelProxyDetailLayer.rightHighlightRect =
        MapRect(oldHeadRect, targetHeadRect, scene.rightEyeHighlightRect);
    scene.modelProxyDetailLayer.noseRect = MapRect(oldHeadRect, targetHeadRect, scene.noseRect);
    scene.modelProxyDetailLayer.mouthRect = MapRect(oldHeadRect, targetHeadRect, scene.mouthRect);
    scene.modelProxyDetailLayer.leftBlushRect = MapRect(oldHeadRect, targetHeadRect, scene.leftBlushRect);
    scene.modelProxyDetailLayer.rightBlushRect = MapRect(oldHeadRect, targetHeadRect, scene.rightBlushRect);
    scene.modelProxyDetailLayer.mouthStartDeg = scene.mouthStartDeg;
    scene.modelProxyDetailLayer.mouthSweepDeg = scene.mouthSweepDeg;
    MapLineArray(
        scene.leftWhiskerStart,
        scene.leftWhiskerEnd,
        oldHeadRect,
        targetHeadRect,
        &scene.modelProxyDetailLayer.leftWhiskerStart,
        &scene.modelProxyDetailLayer.leftWhiskerEnd,
        0.52f);
    MapLineArray(
        scene.rightWhiskerStart,
        scene.rightWhiskerEnd,
        oldHeadRect,
        targetHeadRect,
        &scene.modelProxyDetailLayer.rightWhiskerStart,
        &scene.modelProxyDetailLayer.rightWhiskerEnd,
        0.52f);

    const float dominanceMix = 0.58f + scene.proxyDominance * 0.42f;
    scene.modelProxyDetailLayer.eyeAlphaScale = 0.92f + 0.20f * dominanceMix;
    scene.modelProxyDetailLayer.mouthAlphaScale = 0.90f + 0.20f * dominanceMix;
    scene.modelProxyDetailLayer.blushAlphaScale = 0.88f + 0.18f * dominanceMix;
    scene.modelProxyDetailLayer.whiskerStrokeScale = 1.04f + 0.18f * dominanceMix;
    scene.modelProxyDetailLayer.highlightAlphaScale = 0.94f + 0.18f * dominanceMix;

    scene.previewDetailAlphaScale *= std::max(0.20f, 0.50f - scene.proxyDominance * 0.18f);
}

} // namespace mousefx::windows
