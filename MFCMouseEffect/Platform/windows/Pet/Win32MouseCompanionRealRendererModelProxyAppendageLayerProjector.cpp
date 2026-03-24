#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyAppendageLayerProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

Gdiplus::RectF BlendRect(
    const Gdiplus::RectF& current,
    const Gdiplus::RectF& target,
    float mix) {
    return Gdiplus::RectF(
        current.X + (target.X - current.X) * mix,
        current.Y + (target.Y - current.Y) * mix,
        current.Width + (target.Width - current.Width) * mix,
        current.Height + (target.Height - current.Height) * mix);
}

Gdiplus::RectF ExpandRect(const Gdiplus::RectF& rect, float dx, float dy) {
    return Gdiplus::RectF(
        rect.X - dx,
        rect.Y - dy,
        rect.Width + dx * 2.0f,
        rect.Height + dy * 2.0f);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyAppendageLayerProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    scene.modelProxyAppendageLayer = Win32MouseCompanionRealRendererModelProxyAppendageLayer{};
    if (!scene.modelProxyFrameLayer.visible) {
        return;
    }

    const auto& frame = scene.modelProxyFrameLayer;
    const float dominanceMix = 0.60f + scene.proxyDominance * 0.40f;
    scene.modelProxyAppendageLayer.visible = true;
    scene.modelProxyAppendageLayer.tailRootCuffRect = BlendRect(
        scene.tailRootCuffRect,
        Gdiplus::RectF(
            frame.tailRect.X - frame.tailRect.Width * 0.06f,
            frame.tailRect.Y + frame.tailRect.Height * 0.16f,
            frame.tailRect.Width * 0.34f,
            frame.tailRect.Height * 0.34f),
        0.48f);
    scene.modelProxyAppendageLayer.tailBridgeRect = BlendRect(scene.tailBridgeRect, ExpandRect(frame.tailRect, -frame.tailRect.Width * 0.12f, -frame.tailRect.Height * 0.22f), 0.34f);
    scene.modelProxyAppendageLayer.tailMidContourRect = BlendRect(scene.tailMidContourRect, ExpandRect(frame.tailRect, -frame.tailRect.Width * 0.06f, -frame.tailRect.Height * 0.14f), 0.36f);
    scene.modelProxyAppendageLayer.tailTipBridgeRect = BlendRect(
        scene.tailTipBridgeRect,
        Gdiplus::RectF(
            frame.tailRect.GetRight() - frame.tailRect.Width * 0.34f,
            frame.tailRect.Y + frame.tailRect.Height * 0.30f,
            frame.tailRect.Width * 0.22f,
            frame.tailRect.Height * 0.22f),
        0.42f);
    scene.modelProxyAppendageLayer.tailTipRect = BlendRect(
        scene.tailTipRect,
        Gdiplus::RectF(
            frame.tailRect.GetRight() - frame.tailRect.Width * 0.18f,
            frame.tailRect.Y + frame.tailRect.Height * 0.24f,
            frame.tailRect.Width * 0.18f,
            frame.tailRect.Height * 0.18f),
        0.48f);

    scene.modelProxyAppendageLayer.leftLegSilhouetteBridgeRect =
        BlendRect(scene.leftLegSilhouetteBridgeRect, ExpandRect(frame.leftLegRect, -frame.leftLegRect.Width * 0.18f, -frame.leftLegRect.Height * 0.18f), 0.36f);
    scene.modelProxyAppendageLayer.rightLegSilhouetteBridgeRect =
        BlendRect(scene.rightLegSilhouetteBridgeRect, ExpandRect(frame.rightLegRect, -frame.rightLegRect.Width * 0.18f, -frame.rightLegRect.Height * 0.18f), 0.36f);
    scene.modelProxyAppendageLayer.leftLegCadenceBridgeRect =
        BlendRect(scene.leftLegCadenceBridgeRect, ExpandRect(frame.leftLegRect, -frame.leftLegRect.Width * 0.12f, -frame.leftLegRect.Height * 0.12f), 0.34f);
    scene.modelProxyAppendageLayer.rightLegCadenceBridgeRect =
        BlendRect(scene.rightLegCadenceBridgeRect, ExpandRect(frame.rightLegRect, -frame.rightLegRect.Width * 0.12f, -frame.rightLegRect.Height * 0.12f), 0.34f);
    scene.modelProxyAppendageLayer.leftLegRootCuffRect =
        BlendRect(scene.leftLegRootCuffRect, ExpandRect(frame.leftLegRect, -frame.leftLegRect.Width * 0.08f, -frame.leftLegRect.Height * 0.06f), 0.38f);
    scene.modelProxyAppendageLayer.rightLegRootCuffRect =
        BlendRect(scene.rightLegRootCuffRect, ExpandRect(frame.rightLegRect, -frame.rightLegRect.Width * 0.08f, -frame.rightLegRect.Height * 0.06f), 0.38f);
    scene.modelProxyAppendageLayer.leftLegPadRect =
        BlendRect(scene.leftLegPadRect, Gdiplus::RectF(frame.leftLegRect.X, frame.leftLegRect.GetBottom() - frame.leftLegRect.Height * 0.18f, frame.leftLegRect.Width * 0.90f, frame.leftLegRect.Height * 0.16f), 0.42f);
    scene.modelProxyAppendageLayer.rightLegPadRect =
        BlendRect(scene.rightLegPadRect, Gdiplus::RectF(frame.rightLegRect.X, frame.rightLegRect.GetBottom() - frame.rightLegRect.Height * 0.18f, frame.rightLegRect.Width * 0.90f, frame.rightLegRect.Height * 0.16f), 0.42f);

    scene.modelProxyAppendageLayer.leftHandSilhouetteBridgeRect =
        BlendRect(scene.leftHandSilhouetteBridgeRect, ExpandRect(frame.leftHandRect, -frame.leftHandRect.Width * 0.18f, -frame.leftHandRect.Height * 0.18f), 0.34f);
    scene.modelProxyAppendageLayer.rightHandSilhouetteBridgeRect =
        BlendRect(scene.rightHandSilhouetteBridgeRect, ExpandRect(frame.rightHandRect, -frame.rightHandRect.Width * 0.18f, -frame.rightHandRect.Height * 0.18f), 0.34f);
    scene.modelProxyAppendageLayer.leftHandCadenceBridgeRect =
        BlendRect(scene.leftHandCadenceBridgeRect, ExpandRect(frame.leftHandRect, -frame.leftHandRect.Width * 0.12f, -frame.leftHandRect.Height * 0.12f), 0.34f);
    scene.modelProxyAppendageLayer.rightHandCadenceBridgeRect =
        BlendRect(scene.rightHandCadenceBridgeRect, ExpandRect(frame.rightHandRect, -frame.rightHandRect.Width * 0.12f, -frame.rightHandRect.Height * 0.12f), 0.34f);
    scene.modelProxyAppendageLayer.leftHandRootCuffRect =
        BlendRect(scene.leftHandRootCuffRect, ExpandRect(frame.leftHandRect, -frame.leftHandRect.Width * 0.08f, -frame.leftHandRect.Height * 0.06f), 0.36f);
    scene.modelProxyAppendageLayer.rightHandRootCuffRect =
        BlendRect(scene.rightHandRootCuffRect, ExpandRect(frame.rightHandRect, -frame.rightHandRect.Width * 0.08f, -frame.rightHandRect.Height * 0.06f), 0.36f);
    scene.modelProxyAppendageLayer.leftHandPadRect =
        BlendRect(scene.leftHandPadRect, Gdiplus::RectF(frame.leftHandRect.X, frame.leftHandRect.GetBottom() - frame.leftHandRect.Height * 0.18f, frame.leftHandRect.Width * 0.90f, frame.leftHandRect.Height * 0.16f), 0.40f);
    scene.modelProxyAppendageLayer.rightHandPadRect =
        BlendRect(scene.rightHandPadRect, Gdiplus::RectF(frame.rightHandRect.X, frame.rightHandRect.GetBottom() - frame.rightHandRect.Height * 0.18f, frame.rightHandRect.Width * 0.90f, frame.rightHandRect.Height * 0.16f), 0.40f);

    scene.modelProxyAppendageLayer.rearAlphaScale = 0.92f + 0.16f * dominanceMix;
    scene.modelProxyAppendageLayer.accentAlphaScale = 0.90f + 0.18f * dominanceMix;
    scene.modelProxyAppendageLayer.strokeAlphaScale = 0.88f + 0.16f * dominanceMix;

    scene.previewAppendageAlphaScale *= std::max(0.16f, 0.38f - scene.proxyDominance * 0.12f);
}

} // namespace mousefx::windows
