#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAdornmentBuilder.h"

#include <cmath>

namespace mousefx::windows {
namespace {

std::array<Gdiplus::PointF, 5> BuildStarPoints(
    float centerX,
    float centerY,
    float outerRadius,
    float innerRadius,
    float innerYOffsetRatio) {
    std::array<Gdiplus::PointF, 5> points{};
    for (size_t i = 0; i < points.size(); ++i) {
        const float angle = -3.1415926f * 0.5f + static_cast<float>(i) * (3.1415926f * 2.0f / 5.0f);
        const float x = centerX + std::cos(angle) * outerRadius;
        const float y = centerY + std::sin(angle) * outerRadius;
        points[i] = Gdiplus::PointF(x, y - innerRadius * innerYOffsetRatio);
    }
    return points;
}

} // namespace

void BuildWin32MouseCompanionRealRendererAdornment(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    const Win32MouseCompanionRealRendererStyleProfile& style,
    const Win32MouseCompanionRealRendererLayoutMetrics& metrics,
    Win32MouseCompanionRealRendererScene& scene) {
    const float badgeY = scene.pedestalRect.GetBottom() + metrics.bodyHeight * style.badgeYOffsetRatio;
    const float badgeW = metrics.bodyWidth * style.badgeWidthRatio;
    const float badgeH = metrics.bodyHeight * style.badgeHeightRatio;
    for (size_t i = 0; i < scene.laneBadgeRects.size(); ++i) {
        const float x = scene.centerX - badgeW * style.badgeStartXScale + static_cast<float>(i) * (badgeW * style.badgeGapScale);
        scene.laneBadgeRects[i] = Gdiplus::RectF(x, badgeY, badgeW, badgeH);
    }
    scene.laneReady = {
        runtime.assets->modelReady,
        runtime.assets->actionLibraryReady,
        runtime.assets->appearanceProfileReady,
    };

    scene.poseBadgeVisible = runtime.poseFrameAvailable || runtime.poseBindingConfigured;
    scene.poseBadgeRect = Gdiplus::RectF(
        scene.headRect.GetRight() - scene.headRect.Width * style.poseBadgeXRatio,
        scene.headRect.Y - scene.headRect.Height * style.poseBadgeYRatio,
        scene.headRect.Width * style.poseBadgeSizeRatio,
        scene.headRect.Width * style.poseBadgeSizeRatio);

    scene.accessoryVisible = !runtime.assets->appearanceAccessoryIds.empty();
    if (!scene.accessoryVisible) {
        return;
    }

    scene.accessoryStar = BuildStarPoints(
        scene.headRect.GetRight() - scene.headRect.Width * style.accessoryXRatio,
        scene.headRect.Y + scene.headRect.Height * style.accessoryYRatio,
        scene.headRect.Width * style.accessoryOuterRatio,
        scene.headRect.Width * style.accessoryInnerRatio,
        style.accessoryStarInnerYOffsetRatio);
}

} // namespace mousefx::windows
