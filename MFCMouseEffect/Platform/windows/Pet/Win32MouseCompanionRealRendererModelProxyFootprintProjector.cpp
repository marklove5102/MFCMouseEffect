#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyFootprintProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

Gdiplus::RectF BuildHullBounds(const std::vector<Gdiplus::PointF>& points) {
    if (points.empty()) {
        return Gdiplus::RectF{};
    }

    float minX = points.front().X;
    float maxX = points.front().X;
    float minY = points.front().Y;
    float maxY = points.front().Y;
    for (const auto& point : points) {
        minX = std::min(minX, point.X);
        maxX = std::max(maxX, point.X);
        minY = std::min(minY, point.Y);
        maxY = std::max(maxY, point.Y);
    }

    return Gdiplus::RectF(minX, minY, maxX - minX, maxY - minY);
}

float Blend(float current, float target, float factor) {
    return current + (target - current) * factor;
}

void BlendRect(
    Gdiplus::RectF* rect,
    const Gdiplus::RectF& target,
    float factor) {
    if (rect == nullptr) {
        return;
    }
    rect->X = Blend(rect->X, target.X, factor);
    rect->Y = Blend(rect->Y, target.Y, factor);
    rect->Width = Blend(rect->Width, target.Width, factor);
    rect->Height = Blend(rect->Height, target.Height, factor);
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyFootprintProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    if (!scene.modelProxyVisible || scene.modelProxyHull.size() < 3) {
        return;
    }

    const Gdiplus::RectF hullBounds = BuildHullBounds(scene.modelProxyHull);
    if (hullBounds.Width <= 0.0f || hullBounds.Height <= 0.0f) {
        return;
    }

    const float centerX = hullBounds.X + hullBounds.Width * 0.5f;
    const float baseY = hullBounds.GetBottom();
    const Gdiplus::RectF targetShadow(
        centerX - hullBounds.Width * 0.34f,
        baseY - hullBounds.Height * 0.02f,
        std::max(scene.shadowRect.Width, hullBounds.Width * 0.68f),
        std::max(scene.shadowRect.Height, hullBounds.Height * 0.28f));
    const Gdiplus::RectF targetPedestal(
        centerX - hullBounds.Width * 0.42f,
        baseY + hullBounds.Height * 0.04f,
        std::max(scene.pedestalRect.Width, hullBounds.Width * 0.84f),
        std::max(scene.pedestalRect.Height, hullBounds.Height * 0.20f));
    const Gdiplus::RectF targetGlow(
        centerX - hullBounds.Width * 0.58f,
        hullBounds.Y - hullBounds.Height * 0.16f,
        std::max(scene.glowRect.Width, hullBounds.Width * 1.16f),
        std::max(scene.glowRect.Height, hullBounds.Height * 1.28f));

    BlendRect(&scene.shadowRect, targetShadow, 0.62f);
    BlendRect(&scene.pedestalRect, targetPedestal, 0.58f);
    BlendRect(&scene.glowRect, targetGlow, 0.36f);
    scene.shadowAlphaScale = std::min(1.18f, scene.shadowAlphaScale * 1.04f);
    scene.pedestalAlphaScale = std::min(1.16f, scene.pedestalAlphaScale * 1.03f);
    scene.glowAlpha = std::min(255.0f, scene.glowAlpha + 10.0f);
}

} // namespace mousefx::windows
