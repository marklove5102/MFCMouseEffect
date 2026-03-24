#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyActionAnchorProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

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

Gdiplus::PointF RectCenter(const Gdiplus::RectF& rect) {
    return Gdiplus::PointF(
        rect.X + rect.Width * 0.5f,
        rect.Y + rect.Height * 0.5f);
}

float ResolveAnchorMix(const Win32MouseCompanionRealRendererScene& scene) {
    float mix = std::clamp(scene.proxyDominance, 0.0f, 1.0f) * 0.28f;
    if (scene.modelProxyActionLayer.holdShellVisible) {
        mix += 0.18f;
    }
    if (scene.modelProxyActionLayer.dragShellVisible) {
        mix += 0.16f;
    }
    if (scene.modelProxyActionLayer.followShellVisible) {
        mix += 0.14f;
    }
    if (scene.modelProxyActionLayer.scrollShellVisible) {
        mix += 0.10f;
    }
    if (scene.modelProxyActionLayer.clickShellVisible) {
        mix += 0.06f;
    }
    return std::clamp(mix, 0.0f, 0.76f);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyActionAnchorProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    if (!scene.modelProxyVisible) {
        return;
    }

    const float mix = ResolveAnchorMix(scene);
    if (mix <= 0.0f) {
        return;
    }

    if (scene.modelProxyAppendageLayer.visible) {
        const Gdiplus::PointF tailCenter =
            RectCenter(scene.modelProxyAppendageLayer.tailBridgeRect);
        const Gdiplus::PointF handCenter = Gdiplus::PointF(
            (RectCenter(scene.modelProxyAppendageLayer.leftHandCadenceBridgeRect).X +
             RectCenter(scene.modelProxyAppendageLayer.rightHandCadenceBridgeRect).X) *
                0.5f,
            (RectCenter(scene.modelProxyAppendageLayer.leftHandCadenceBridgeRect).Y +
             RectCenter(scene.modelProxyAppendageLayer.rightHandCadenceBridgeRect).Y) *
                0.5f);
        const Gdiplus::PointF targetAppendage = Gdiplus::PointF(
            Blend(tailCenter.X, handCenter.X, 0.58f),
            Blend(tailCenter.Y, handCenter.Y, 0.58f));
        scene.appendageAnchor = BlendPoint(scene.appendageAnchor, targetAppendage, mix);
    }

    if (scene.modelProxyActionLayer.holdShellVisible) {
        scene.overlayAnchor = BlendPoint(
            scene.overlayAnchor,
            RectCenter(scene.modelProxyActionLayer.holdShellRect),
            mix * 0.78f);
    } else if (scene.modelProxyActionLayer.scrollShellVisible) {
        scene.overlayAnchor = BlendPoint(
            scene.overlayAnchor,
            RectCenter(scene.modelProxyActionLayer.scrollShellRect),
            mix * 0.68f);
    } else if (scene.modelProxyActionLayer.clickShellVisible) {
        scene.overlayAnchor = BlendPoint(
            scene.overlayAnchor,
            RectCenter(scene.modelProxyActionLayer.clickShellRect),
            mix * 0.56f);
    }

    if (scene.modelProxyActionLayer.followShellVisible) {
        scene.groundingAnchor = BlendPoint(
            scene.groundingAnchor,
            RectCenter(scene.modelProxyActionLayer.followShellRects.back()),
            mix * 0.72f);
    } else if (scene.modelProxyActionLayer.dragShellVisible) {
        const Gdiplus::PointF dragMid = Gdiplus::PointF(
            (scene.modelProxyActionLayer.dragShellStart.X +
             scene.modelProxyActionLayer.dragShellEnd.X) *
                0.5f,
            (scene.modelProxyActionLayer.dragShellStart.Y +
             scene.modelProxyActionLayer.dragShellEnd.Y) *
                0.5f);
        scene.groundingAnchor = BlendPoint(
            scene.groundingAnchor,
            dragMid,
            mix * 0.64f);
    }
}

} // namespace mousefx::windows
