#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyEarProjector.h"

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

void BlendRect(
    Gdiplus::RectF* rect,
    const Gdiplus::RectF& mapped,
    float mix) {
    if (rect == nullptr) {
        return;
    }
    rect->X = Blend(rect->X, mapped.X, mix);
    rect->Y = Blend(rect->Y, mapped.Y, mix);
    rect->Width = Blend(rect->Width, mapped.Width, mix);
    rect->Height = Blend(rect->Height, mapped.Height, mix);
}

template <size_t N>
void BlendPointArray(
    std::array<Gdiplus::PointF, N>* points,
    const std::array<Gdiplus::PointF, N>& mapped,
    float mix) {
    if (points == nullptr) {
        return;
    }
    for (size_t i = 0; i < points->size(); ++i) {
        (*points)[i] = BlendPoint((*points)[i], mapped[i], mix);
    }
}

std::array<Gdiplus::PointF, 4> MapEarPolygon(
    const std::array<Gdiplus::PointF, 4>& ear,
    const Gdiplus::RectF& from,
    const Gdiplus::RectF& to,
    float tipLift) {
    std::array<Gdiplus::PointF, 4> mapped = {{
        MapPoint(from, to, ear[0]),
        MapPoint(from, to, ear[1]),
        MapPoint(from, to, ear[2]),
        MapPoint(from, to, ear[3]),
    }};
    mapped[2].Y -= tipLift;
    return mapped;
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyEarProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    const auto* headSilhouette = FindHeadSilhouette(scene);
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

    const float rectMix = 0.32f;
    const float pointMix = 0.36f;
    const float tipLift = targetHeadRect.Height * 0.05f;

    BlendRect(
        &scene.leftEarRootCuffRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftEarRootCuffRect),
        rectMix);
    BlendRect(
        &scene.rightEarRootCuffRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightEarRootCuffRect),
        rectMix);
    BlendRect(
        &scene.leftEarOcclusionCapRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftEarOcclusionCapRect),
        rectMix);
    BlendRect(
        &scene.rightEarOcclusionCapRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightEarOcclusionCapRect),
        rectMix);
    BlendRect(
        &scene.leftEarSkullBridgeRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftEarSkullBridgeRect),
        rectMix);
    BlendRect(
        &scene.rightEarSkullBridgeRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightEarSkullBridgeRect),
        rectMix);

    BlendPointArray(
        &scene.leftEar,
        MapEarPolygon(scene.leftEar, oldHeadRect, targetHeadRect, tipLift),
        pointMix);
    BlendPointArray(
        &scene.rightEar,
        MapEarPolygon(scene.rightEar, oldHeadRect, targetHeadRect, tipLift),
        pointMix);
}

} // namespace mousefx::windows
