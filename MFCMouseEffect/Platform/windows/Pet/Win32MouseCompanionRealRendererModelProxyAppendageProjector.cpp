#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyAppendageProjector.h"

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

Gdiplus::RectF BuildLimbClusterBounds(const Win32MouseCompanionRealRendererScene& scene) {
    const float left = std::min(
        std::min(scene.leftHandRect.X, scene.rightHandRect.X),
        std::min(scene.leftLegRect.X, scene.rightLegRect.X));
    const float right = std::max(
        std::max(scene.leftHandRect.GetRight(), scene.rightHandRect.GetRight()),
        std::max(scene.leftLegRect.GetRight(), scene.rightLegRect.GetRight()));
    const float top = std::min(
        std::min(scene.leftHandRect.Y, scene.rightHandRect.Y),
        std::min(scene.leftLegRect.Y, scene.rightLegRect.Y));
    const float bottom = std::max(
        std::max(scene.leftHandRect.GetBottom(), scene.rightHandRect.GetBottom()),
        std::max(scene.leftLegRect.GetBottom(), scene.rightLegRect.GetBottom()));
    return Gdiplus::RectF(left, top, right - left, bottom - top);
}

Gdiplus::RectF BuildTargetLimbClusterBounds(
    const Gdiplus::RectF& currentBounds,
    const Win32MouseCompanionRealRendererModelProxySilhouette& bodySilhouette,
    const Win32MouseCompanionRealRendererModelProxySilhouette& groundingSilhouette) {
    const float targetCenterX = Blend(
        currentBounds.X + currentBounds.Width * 0.5f,
        bodySilhouette.bounds.X + bodySilhouette.bounds.Width * 0.5f,
        0.70f);
    const float targetTop = Blend(
        currentBounds.Y,
        bodySilhouette.bounds.GetBottom() - bodySilhouette.bounds.Height * 0.06f,
        0.72f);
    const float targetBottom = Blend(
        currentBounds.GetBottom(),
        groundingSilhouette.bounds.Y + groundingSilhouette.bounds.Height * 0.34f,
        0.68f);
    const float targetWidth = std::max(
        currentBounds.Width * 0.92f,
        std::max(
            bodySilhouette.bounds.Width * 0.96f,
            groundingSilhouette.bounds.Width * 0.78f));
    const float targetHeight = std::max(
        currentBounds.Height * 0.94f,
        targetBottom - targetTop);
    return Gdiplus::RectF(
        targetCenterX - targetWidth * 0.5f,
        targetTop,
        targetWidth,
        targetHeight);
}

void ProjectLimbCluster(
    const Gdiplus::RectF& oldLimbBounds,
    const Gdiplus::RectF& targetLimbBounds,
    Win32MouseCompanionRealRendererScene& scene) {
    const float mix = 0.36f;
    BlendRect(&scene.leftHandRect, MapRect(oldLimbBounds, targetLimbBounds, scene.leftHandRect), mix);
    BlendRect(&scene.rightHandRect, MapRect(oldLimbBounds, targetLimbBounds, scene.rightHandRect), mix);
    BlendRect(&scene.leftLegRect, MapRect(oldLimbBounds, targetLimbBounds, scene.leftLegRect), mix);
    BlendRect(&scene.rightLegRect, MapRect(oldLimbBounds, targetLimbBounds, scene.rightLegRect), mix);
    BlendRect(&scene.leftHandRootCuffRect, MapRect(oldLimbBounds, targetLimbBounds, scene.leftHandRootCuffRect), mix);
    BlendRect(&scene.rightHandRootCuffRect, MapRect(oldLimbBounds, targetLimbBounds, scene.rightHandRootCuffRect), mix);
    BlendRect(&scene.leftLegRootCuffRect, MapRect(oldLimbBounds, targetLimbBounds, scene.leftLegRootCuffRect), mix);
    BlendRect(&scene.rightLegRootCuffRect, MapRect(oldLimbBounds, targetLimbBounds, scene.rightLegRootCuffRect), mix);
    BlendRect(
        &scene.leftHandSilhouetteBridgeRect,
        MapRect(oldLimbBounds, targetLimbBounds, scene.leftHandSilhouetteBridgeRect),
        mix);
    BlendRect(
        &scene.rightHandSilhouetteBridgeRect,
        MapRect(oldLimbBounds, targetLimbBounds, scene.rightHandSilhouetteBridgeRect),
        mix);
    BlendRect(
        &scene.leftLegSilhouetteBridgeRect,
        MapRect(oldLimbBounds, targetLimbBounds, scene.leftLegSilhouetteBridgeRect),
        mix);
    BlendRect(
        &scene.rightLegSilhouetteBridgeRect,
        MapRect(oldLimbBounds, targetLimbBounds, scene.rightLegSilhouetteBridgeRect),
        mix);
    BlendRect(
        &scene.leftHandCadenceBridgeRect,
        MapRect(oldLimbBounds, targetLimbBounds, scene.leftHandCadenceBridgeRect),
        mix);
    BlendRect(
        &scene.rightHandCadenceBridgeRect,
        MapRect(oldLimbBounds, targetLimbBounds, scene.rightHandCadenceBridgeRect),
        mix);
    BlendRect(
        &scene.leftLegCadenceBridgeRect,
        MapRect(oldLimbBounds, targetLimbBounds, scene.leftLegCadenceBridgeRect),
        mix);
    BlendRect(
        &scene.rightLegCadenceBridgeRect,
        MapRect(oldLimbBounds, targetLimbBounds, scene.rightLegCadenceBridgeRect),
        mix);
    BlendRect(&scene.leftHandPadRect, MapRect(oldLimbBounds, targetLimbBounds, scene.leftHandPadRect), mix);
    BlendRect(&scene.rightHandPadRect, MapRect(oldLimbBounds, targetLimbBounds, scene.rightHandPadRect), mix);
    BlendRect(&scene.leftLegPadRect, MapRect(oldLimbBounds, targetLimbBounds, scene.leftLegPadRect), mix);
    BlendRect(&scene.rightLegPadRect, MapRect(oldLimbBounds, targetLimbBounds, scene.rightLegPadRect), mix);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyAppendageProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    const auto* bodySilhouette = FindSilhouette(scene, "body");
    const auto* groundingSilhouette = FindSilhouette(scene, "grounding");
    if (bodySilhouette == nullptr || groundingSilhouette == nullptr ||
        bodySilhouette->bounds.Width <= 0.0f || groundingSilhouette->bounds.Width <= 0.0f) {
        return;
    }

    const Gdiplus::RectF oldLimbBounds = BuildLimbClusterBounds(scene);
    if (oldLimbBounds.Width <= 0.0f || oldLimbBounds.Height <= 0.0f) {
        return;
    }

    const Gdiplus::RectF targetLimbBounds =
        BuildTargetLimbClusterBounds(oldLimbBounds, *bodySilhouette, *groundingSilhouette);
    ProjectLimbCluster(oldLimbBounds, targetLimbBounds, scene);
    scene.groundingAnchor = Gdiplus::PointF(
        targetLimbBounds.X + targetLimbBounds.Width * 0.5f,
        targetLimbBounds.GetBottom() - targetLimbBounds.Height * 0.04f);
}

} // namespace mousefx::windows
