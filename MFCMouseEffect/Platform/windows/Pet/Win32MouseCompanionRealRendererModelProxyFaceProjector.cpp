#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyFaceProjector.h"

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
    const Gdiplus::PointF bottomRight = MapPoint(
        from,
        to,
        Gdiplus::PointF(rect.GetRight(), rect.GetBottom()));
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

void BlendPoint(
    Gdiplus::PointF* point,
    const Gdiplus::PointF& mapped,
    float mix) {
    if (point == nullptr) {
        return;
    }
    point->X = Blend(point->X, mapped.X, mix);
    point->Y = Blend(point->Y, mapped.Y, mix);
}

template <size_t N>
void BlendPointArray(
    std::array<Gdiplus::PointF, N>* points,
    const Gdiplus::RectF& oldHeadRect,
    const Gdiplus::RectF& targetHeadRect,
    float mix) {
    if (points == nullptr) {
        return;
    }
    for (auto& point : *points) {
        BlendPoint(&point, MapPoint(oldHeadRect, targetHeadRect, point), mix);
    }
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyFaceProjector(
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
        std::max(oldHeadRect.Width * 0.78f, headSilhouette->bounds.Width),
        std::max(oldHeadRect.Height * 0.78f, headSilhouette->bounds.Height));
    const float mix = 0.34f;

    BlendRect(&scene.leftEyeRect, MapRect(oldHeadRect, targetHeadRect, scene.leftEyeRect), mix);
    BlendRect(&scene.rightEyeRect, MapRect(oldHeadRect, targetHeadRect, scene.rightEyeRect), mix);
    BlendRect(&scene.leftPupilRect, MapRect(oldHeadRect, targetHeadRect, scene.leftPupilRect), mix);
    BlendRect(&scene.rightPupilRect, MapRect(oldHeadRect, targetHeadRect, scene.rightPupilRect), mix);
    BlendRect(
        &scene.leftEyeHighlightRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftEyeHighlightRect),
        mix);
    BlendRect(
        &scene.rightEyeHighlightRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightEyeHighlightRect),
        mix);
    BlendRect(&scene.noseRect, MapRect(oldHeadRect, targetHeadRect, scene.noseRect), mix);
    BlendRect(&scene.mouthRect, MapRect(oldHeadRect, targetHeadRect, scene.mouthRect), mix);
    BlendRect(&scene.leftBlushRect, MapRect(oldHeadRect, targetHeadRect, scene.leftBlushRect), mix);
    BlendRect(&scene.rightBlushRect, MapRect(oldHeadRect, targetHeadRect, scene.rightBlushRect), mix);
    BlendRect(
        &scene.leftCheekContourRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftCheekContourRect),
        mix);
    BlendRect(
        &scene.rightCheekContourRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightCheekContourRect),
        mix);
    BlendRect(&scene.jawContourRect, MapRect(oldHeadRect, targetHeadRect, scene.jawContourRect), mix);
    BlendRect(&scene.muzzlePadRect, MapRect(oldHeadRect, targetHeadRect, scene.muzzlePadRect), mix);
    BlendRect(
        &scene.foreheadPadRect,
        MapRect(oldHeadRect, targetHeadRect, scene.foreheadPadRect),
        mix);
    BlendRect(&scene.crownPadRect, MapRect(oldHeadRect, targetHeadRect, scene.crownPadRect), mix);
    BlendRect(
        &scene.leftParietalBridgeRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftParietalBridgeRect),
        mix);
    BlendRect(
        &scene.rightParietalBridgeRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightParietalBridgeRect),
        mix);
    BlendRect(
        &scene.leftEarSkullBridgeRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftEarSkullBridgeRect),
        mix);
    BlendRect(
        &scene.rightEarSkullBridgeRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightEarSkullBridgeRect),
        mix);
    BlendRect(
        &scene.leftOccipitalContourRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftOccipitalContourRect),
        mix);
    BlendRect(
        &scene.rightOccipitalContourRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightOccipitalContourRect),
        mix);
    BlendRect(
        &scene.leftTempleContourRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftTempleContourRect),
        mix);
    BlendRect(
        &scene.rightTempleContourRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightTempleContourRect),
        mix);
    BlendRect(
        &scene.leftUnderEyeContourRect,
        MapRect(oldHeadRect, targetHeadRect, scene.leftUnderEyeContourRect),
        mix);
    BlendRect(
        &scene.rightUnderEyeContourRect,
        MapRect(oldHeadRect, targetHeadRect, scene.rightUnderEyeContourRect),
        mix);
    BlendRect(&scene.noseBridgeRect, MapRect(oldHeadRect, targetHeadRect, scene.noseBridgeRect), mix);

    BlendPoint(&scene.leftBrowStart, MapPoint(oldHeadRect, targetHeadRect, scene.leftBrowStart), mix);
    BlendPoint(&scene.leftBrowEnd, MapPoint(oldHeadRect, targetHeadRect, scene.leftBrowEnd), mix);
    BlendPoint(&scene.rightBrowStart, MapPoint(oldHeadRect, targetHeadRect, scene.rightBrowStart), mix);
    BlendPoint(&scene.rightBrowEnd, MapPoint(oldHeadRect, targetHeadRect, scene.rightBrowEnd), mix);
    BlendPointArray(&scene.leftWhiskerStart, oldHeadRect, targetHeadRect, mix);
    BlendPointArray(&scene.leftWhiskerEnd, oldHeadRect, targetHeadRect, mix);
    BlendPointArray(&scene.rightWhiskerStart, oldHeadRect, targetHeadRect, mix);
    BlendPointArray(&scene.rightWhiskerEnd, oldHeadRect, targetHeadRect, mix);
}

} // namespace mousefx::windows
