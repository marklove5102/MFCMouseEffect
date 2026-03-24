#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyFrameProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

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

void ProjectBodyCluster(
    const Gdiplus::RectF& oldBodyRect,
    const Gdiplus::RectF& targetBodyRect,
    Win32MouseCompanionRealRendererScene& scene) {
    const float mix = 0.30f;
    BlendRect(&scene.bodyRect, targetBodyRect, mix);
    BlendRect(&scene.chestRect, MapRect(oldBodyRect, targetBodyRect, scene.chestRect), mix);
    BlendRect(&scene.neckBridgeRect, MapRect(oldBodyRect, targetBodyRect, scene.neckBridgeRect), mix);
    BlendRect(
        &scene.leftHeadShoulderBridgeRect,
        MapRect(oldBodyRect, targetBodyRect, scene.leftHeadShoulderBridgeRect),
        mix);
    BlendRect(
        &scene.rightHeadShoulderBridgeRect,
        MapRect(oldBodyRect, targetBodyRect, scene.rightHeadShoulderBridgeRect),
        mix);
    BlendRect(
        &scene.leftShoulderPatchRect,
        MapRect(oldBodyRect, targetBodyRect, scene.leftShoulderPatchRect),
        mix);
    BlendRect(
        &scene.rightShoulderPatchRect,
        MapRect(oldBodyRect, targetBodyRect, scene.rightShoulderPatchRect),
        mix);
    BlendRect(
        &scene.leftHipPatchRect,
        MapRect(oldBodyRect, targetBodyRect, scene.leftHipPatchRect),
        mix);
    BlendRect(
        &scene.rightHipPatchRect,
        MapRect(oldBodyRect, targetBodyRect, scene.rightHipPatchRect),
        mix);
    BlendRect(
        &scene.bellyContourRect,
        MapRect(oldBodyRect, targetBodyRect, scene.bellyContourRect),
        mix);
    BlendRect(
        &scene.sternumContourRect,
        MapRect(oldBodyRect, targetBodyRect, scene.sternumContourRect),
        mix);
    BlendRect(
        &scene.upperTorsoContourRect,
        MapRect(oldBodyRect, targetBodyRect, scene.upperTorsoContourRect),
        mix);
    BlendRect(
        &scene.leftTorsoCadenceBridgeRect,
        MapRect(oldBodyRect, targetBodyRect, scene.leftTorsoCadenceBridgeRect),
        mix);
    BlendRect(
        &scene.rightTorsoCadenceBridgeRect,
        MapRect(oldBodyRect, targetBodyRect, scene.rightTorsoCadenceBridgeRect),
        mix);
    BlendRect(
        &scene.leftBackContourRect,
        MapRect(oldBodyRect, targetBodyRect, scene.leftBackContourRect),
        mix);
    BlendRect(
        &scene.rightBackContourRect,
        MapRect(oldBodyRect, targetBodyRect, scene.rightBackContourRect),
        mix);
    BlendRect(
        &scene.leftFlankContourRect,
        MapRect(oldBodyRect, targetBodyRect, scene.leftFlankContourRect),
        mix);
    BlendRect(
        &scene.rightFlankContourRect,
        MapRect(oldBodyRect, targetBodyRect, scene.rightFlankContourRect),
        mix);
    BlendRect(
        &scene.leftTailHaunchBridgeRect,
        MapRect(oldBodyRect, targetBodyRect, scene.leftTailHaunchBridgeRect),
        mix);
    BlendRect(
        &scene.rightTailHaunchBridgeRect,
        MapRect(oldBodyRect, targetBodyRect, scene.rightTailHaunchBridgeRect),
        mix);
}

void ProjectHeadCluster(
    const Gdiplus::RectF& oldHeadRect,
    const Gdiplus::RectF& targetHeadRect,
    Win32MouseCompanionRealRendererScene& scene) {
    BlendRect(&scene.headRect, targetHeadRect, 0.26f);
}

void ProjectAppendageCluster(
    const Gdiplus::RectF& oldTailRect,
    const Gdiplus::RectF& targetTailRect,
    Win32MouseCompanionRealRendererScene& scene) {
    const float mix = 0.34f;
    BlendRect(&scene.tailRect, targetTailRect, mix);
    BlendRect(
        &scene.tailRootCuffRect,
        MapRect(oldTailRect, targetTailRect, scene.tailRootCuffRect),
        mix);
    BlendRect(
        &scene.tailBridgeRect,
        MapRect(oldTailRect, targetTailRect, scene.tailBridgeRect),
        mix);
    BlendRect(
        &scene.tailMidContourRect,
        MapRect(oldTailRect, targetTailRect, scene.tailMidContourRect),
        mix);
    BlendRect(
        &scene.tailTipBridgeRect,
        MapRect(oldTailRect, targetTailRect, scene.tailTipBridgeRect),
        mix);
    BlendRect(
        &scene.tailTipRect,
        MapRect(oldTailRect, targetTailRect, scene.tailTipRect),
        mix);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyFrameProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    const auto* bodySilhouette = FindSilhouette(scene, "body");
    const auto* headSilhouette = FindSilhouette(scene, "head");
    const auto* appendageSilhouette = FindSilhouette(scene, "appendage");

    if (bodySilhouette != nullptr &&
        bodySilhouette->bounds.Width > 0.0f &&
        bodySilhouette->bounds.Height > 0.0f) {
        const Gdiplus::RectF oldBodyRect = scene.bodyRect;
        ProjectBodyCluster(oldBodyRect, bodySilhouette->bounds, scene);
    }

    if (headSilhouette != nullptr &&
        headSilhouette->bounds.Width > 0.0f &&
        headSilhouette->bounds.Height > 0.0f) {
        const Gdiplus::RectF oldHeadRect = scene.headRect;
        ProjectHeadCluster(oldHeadRect, headSilhouette->bounds, scene);
    }

    if (appendageSilhouette != nullptr &&
        appendageSilhouette->bounds.Width > 0.0f &&
        appendageSilhouette->bounds.Height > 0.0f) {
        const Gdiplus::RectF oldTailRect = scene.tailRect;
        ProjectAppendageCluster(oldTailRect, appendageSilhouette->bounds, scene);
    }

    scene.bodyAnchor = Gdiplus::PointF(
        scene.bodyRect.X + scene.bodyRect.Width * 0.5f,
        scene.bodyRect.Y + scene.bodyRect.Height * 0.52f);
    scene.headAnchor = Gdiplus::PointF(
        scene.headRect.X + scene.headRect.Width * 0.5f,
        scene.headRect.Y + scene.headRect.Height * 0.50f);
    scene.appendageAnchor = Gdiplus::PointF(
        scene.tailRect.X + scene.tailRect.Width * 0.52f,
        scene.tailRect.Y + scene.tailRect.Height * 0.48f);
}

} // namespace mousefx::windows
