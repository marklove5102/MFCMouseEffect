#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyAdornmentProjector.h"

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
    const Gdiplus::RectF& from,
    const Gdiplus::RectF& to,
    float mix) {
    if (points == nullptr) {
        return;
    }
    for (auto& point : *points) {
        point = BlendPoint(point, MapPoint(from, to, point), mix);
    }
}

void ProjectBadgeLane(
    const Gdiplus::RectF& pedestalRect,
    std::array<Gdiplus::RectF, 3>* badges) {
    if (badges == nullptr || badges->empty()) {
        return;
    }
    const float width = std::max(10.0f, (*badges)[0].Width);
    const float height = std::max(8.0f, (*badges)[0].Height);
    const float spacing = width * 0.18f;
    const float totalWidth = width * 3.0f + spacing * 2.0f;
    const float startX = pedestalRect.X + (pedestalRect.Width - totalWidth) * 0.5f;
    const float y = pedestalRect.GetBottom() + pedestalRect.Height * 0.18f;
    for (size_t i = 0; i < badges->size(); ++i) {
        (*badges)[i] = Gdiplus::RectF(
            startX + static_cast<float>(i) * (width + spacing),
            y,
            width,
            height);
    }
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyAdornmentProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    const auto* headSilhouette = FindSilhouette(scene, "head");
    const auto* overlaySilhouette = FindSilhouette(scene, "overlay");
    const auto* groundingSilhouette = FindSilhouette(scene, "grounding");

    if (overlaySilhouette != nullptr && overlaySilhouette->bounds.Width > 0.0f &&
        overlaySilhouette->bounds.Height > 0.0f) {
        const Gdiplus::PointF overlayCenter(
            overlaySilhouette->bounds.X + overlaySilhouette->bounds.Width * 0.5f,
            overlaySilhouette->bounds.Y + overlaySilhouette->bounds.Height * 0.5f);
        scene.overlayAnchor = BlendPoint(scene.overlayAnchor, overlayCenter, 0.48f);
        scene.overlayAnchorScale = std::min(1.22f, scene.overlayAnchorScale * 1.03f);
    }

    if (groundingSilhouette != nullptr && groundingSilhouette->bounds.Width > 0.0f &&
        groundingSilhouette->bounds.Height > 0.0f) {
        const Gdiplus::PointF groundingCenter(
            groundingSilhouette->bounds.X + groundingSilhouette->bounds.Width * 0.5f,
            groundingSilhouette->bounds.Y + groundingSilhouette->bounds.Height * 0.72f);
        scene.groundingAnchor = BlendPoint(scene.groundingAnchor, groundingCenter, 0.42f);
        scene.groundingAnchorScale = std::min(1.20f, scene.groundingAnchorScale * 1.03f);
    }

    if (headSilhouette != nullptr && headSilhouette->bounds.Width > 0.0f &&
        headSilhouette->bounds.Height > 0.0f) {
        const Gdiplus::RectF oldHeadRect = scene.headRect;
        const Gdiplus::RectF targetHeadRect(
            headSilhouette->bounds.X,
            headSilhouette->bounds.Y,
            std::max(oldHeadRect.Width * 0.80f, headSilhouette->bounds.Width),
            std::max(oldHeadRect.Height * 0.80f, headSilhouette->bounds.Height));
        BlendRect(&scene.poseBadgeRect, MapRect(oldHeadRect, targetHeadRect, scene.poseBadgeRect), 0.34f);
        BlendRect(&scene.accessoryBounds, MapRect(oldHeadRect, targetHeadRect, scene.accessoryBounds), 0.30f);
        BlendRect(&scene.accessoryMoonInsetRect, MapRect(oldHeadRect, targetHeadRect, scene.accessoryMoonInsetRect), 0.30f);
        BlendRect(&scene.accessoryRibbonCenter, MapRect(oldHeadRect, targetHeadRect, scene.accessoryRibbonCenter), 0.30f);
        BlendPointArray(&scene.accessoryStar, oldHeadRect, targetHeadRect, 0.30f);
        BlendPointArray(&scene.accessoryMoon, oldHeadRect, targetHeadRect, 0.30f);
        BlendPointArray(&scene.accessoryLeaf, oldHeadRect, targetHeadRect, 0.30f);
        BlendPointArray(&scene.accessoryRibbonLeft, oldHeadRect, targetHeadRect, 0.30f);
        BlendPointArray(&scene.accessoryRibbonRight, oldHeadRect, targetHeadRect, 0.30f);
        scene.accessoryLeafVeinStart = BlendPoint(
            scene.accessoryLeafVeinStart,
            MapPoint(oldHeadRect, targetHeadRect, scene.accessoryLeafVeinStart),
            0.30f);
        scene.accessoryLeafVeinEnd = BlendPoint(
            scene.accessoryLeafVeinEnd,
            MapPoint(oldHeadRect, targetHeadRect, scene.accessoryLeafVeinEnd),
            0.30f);
        scene.accessoryRibbonLeftFoldStart = BlendPoint(
            scene.accessoryRibbonLeftFoldStart,
            MapPoint(oldHeadRect, targetHeadRect, scene.accessoryRibbonLeftFoldStart),
            0.30f);
        scene.accessoryRibbonLeftFoldEnd = BlendPoint(
            scene.accessoryRibbonLeftFoldEnd,
            MapPoint(oldHeadRect, targetHeadRect, scene.accessoryRibbonLeftFoldEnd),
            0.30f);
        scene.accessoryRibbonRightFoldStart = BlendPoint(
            scene.accessoryRibbonRightFoldStart,
            MapPoint(oldHeadRect, targetHeadRect, scene.accessoryRibbonRightFoldStart),
            0.30f);
        scene.accessoryRibbonRightFoldEnd = BlendPoint(
            scene.accessoryRibbonRightFoldEnd,
            MapPoint(oldHeadRect, targetHeadRect, scene.accessoryRibbonRightFoldEnd),
            0.30f);
    }

    ProjectBadgeLane(scene.pedestalRect, &scene.laneBadgeRects);
}

} // namespace mousefx::windows
