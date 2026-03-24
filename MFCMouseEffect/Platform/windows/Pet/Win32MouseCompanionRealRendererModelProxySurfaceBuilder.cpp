#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxySurfaceBuilder.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeWorldSpaceProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>
#include <cmath>

namespace mousefx::windows {
namespace {

using WorldSpaceEntry = Win32MouseCompanionRealRendererAssetNodeWorldSpaceEntry;

Gdiplus::Color ResolveSurfaceColor(const std::string& surfaceKey) {
    if (surfaceKey == "head_bridge") {
        return Gdiplus::Color(168, 182, 214, 255);
    }
    if (surfaceKey == "appendage_bridge") {
        return Gdiplus::Color(160, 152, 224, 190);
    }
    if (surfaceKey == "overlay_bridge") {
        return Gdiplus::Color(164, 138, 206, 255);
    }
    if (surfaceKey == "grounding_bridge") {
        return Gdiplus::Color(156, 196, 176, 146);
    }
    return Gdiplus::Color(150, 170, 208, 228);
}

float ResolveBridgeWidth(
    const std::string& logicalNode,
    const Win32MouseCompanionRealRendererScene& scene,
    float confidence) {
    const float bodyWidth = std::max(24.0f, scene.bodyRect.Width);
    const float baseWidth = logicalNode == "head"   ? bodyWidth * 0.12f
        : logicalNode == "appendage"                ? bodyWidth * 0.10f
        : logicalNode == "overlay"                  ? bodyWidth * 0.08f
                                                    : bodyWidth * 0.09f;
    return std::max(6.0f, baseWidth * (0.84f + confidence * 0.28f));
}

Gdiplus::PointF Subtract(const Gdiplus::PointF& left, const Gdiplus::PointF& right) {
    return Gdiplus::PointF(left.X - right.X, left.Y - right.Y);
}

Gdiplus::PointF Add(const Gdiplus::PointF& left, const Gdiplus::PointF& right) {
    return Gdiplus::PointF(left.X + right.X, left.Y + right.Y);
}

Gdiplus::PointF Scale(const Gdiplus::PointF& point, float scale) {
    return Gdiplus::PointF(point.X * scale, point.Y * scale);
}

Gdiplus::PointF NormalizeOrFallback(const Gdiplus::PointF& point) {
    const float magnitude =
        std::sqrt(point.X * point.X + point.Y * point.Y);
    if (magnitude <= 0.001f) {
        return Gdiplus::PointF(0.0f, -1.0f);
    }
    return Gdiplus::PointF(point.X / magnitude, point.Y / magnitude);
}

Gdiplus::PointF Perpendicular(const Gdiplus::PointF& point) {
    return Gdiplus::PointF(-point.Y, point.X);
}

void AppendBridgeSurface(
    const WorldSpaceEntry& bodyEntry,
    const WorldSpaceEntry& branchEntry,
    const Win32MouseCompanionRealRendererScene& scene,
    const char* surfaceKey,
    std::vector<Win32MouseCompanionRealRendererModelProxySurface>* surfaces) {
    if (surfaces == nullptr || !bodyEntry.resolved || !branchEntry.resolved) {
        return;
    }

    const Gdiplus::PointF from(bodyEntry.worldX, bodyEntry.worldY);
    const Gdiplus::PointF to(branchEntry.worldX, branchEntry.worldY);
    const Gdiplus::PointF direction =
        NormalizeOrFallback(Subtract(to, from));
    const Gdiplus::PointF normal = Perpendicular(direction);
    const float fromWidth = ResolveBridgeWidth(branchEntry.logicalNode, scene, bodyEntry.matchConfidence);
    const float toWidth = ResolveBridgeWidth(branchEntry.logicalNode, scene, branchEntry.matchConfidence) * 0.74f;

    Win32MouseCompanionRealRendererModelProxySurface surface{};
    surface.surfaceKey = surfaceKey ? surfaceKey : branchEntry.logicalNode;
    surface.fill = ResolveSurfaceColor(surface.surfaceKey);
    surface.alpha = std::clamp(
        84.0f + (bodyEntry.matchConfidence + branchEntry.matchConfidence) * 58.0f,
        84.0f,
        186.0f);
    surface.polygon = {
        Add(from, Scale(normal, fromWidth)),
        Add(to, Scale(normal, toWidth)),
        Add(to, Scale(normal, -toWidth)),
        Add(from, Scale(normal, -fromWidth)),
    };
    surfaces->push_back(surface);
}

void AppendCoreSurface(
    const Win32MouseCompanionRealRendererAssetNodeWorldSpaceProfile& worldSpaceProfile,
    std::vector<Win32MouseCompanionRealRendererModelProxySurface>* surfaces) {
    if (surfaces == nullptr || !worldSpaceProfile.bodyEntry.resolved) {
        return;
    }

    Win32MouseCompanionRealRendererModelProxySurface surface{};
    surface.surfaceKey = "core_shell";
    surface.fill = ResolveSurfaceColor(surface.surfaceKey);
    surface.alpha = 104.0f;

    auto appendPoint = [&surface](const WorldSpaceEntry& entry) {
        if (!entry.resolved) {
            return;
        }
        surface.polygon.push_back(Gdiplus::PointF(entry.worldX, entry.worldY));
    };

    appendPoint(worldSpaceProfile.headEntry);
    appendPoint(worldSpaceProfile.overlayEntry);
    appendPoint(worldSpaceProfile.groundingEntry);
    appendPoint(worldSpaceProfile.appendageEntry);
    appendPoint(worldSpaceProfile.bodyEntry);

    if (surface.polygon.size() >= 3) {
        surfaces->push_back(surface);
    }
}

} // namespace

void BuildWin32MouseCompanionRealRendererModelProxySurfaces(
    const Win32MouseCompanionRealRendererAssetNodeWorldSpaceProfile& worldSpaceProfile,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.modelProxySurfaces.clear();
    scene.modelProxySurfaces.reserve(5);

    AppendBridgeSurface(
        worldSpaceProfile.bodyEntry,
        worldSpaceProfile.headEntry,
        scene,
        "head_bridge",
        &scene.modelProxySurfaces);
    AppendBridgeSurface(
        worldSpaceProfile.bodyEntry,
        worldSpaceProfile.appendageEntry,
        scene,
        "appendage_bridge",
        &scene.modelProxySurfaces);
    AppendBridgeSurface(
        worldSpaceProfile.bodyEntry,
        worldSpaceProfile.overlayEntry,
        scene,
        "overlay_bridge",
        &scene.modelProxySurfaces);
    AppendBridgeSurface(
        worldSpaceProfile.bodyEntry,
        worldSpaceProfile.groundingEntry,
        scene,
        "grounding_bridge",
        &scene.modelProxySurfaces);
    AppendCoreSurface(worldSpaceProfile, &scene.modelProxySurfaces);
}

} // namespace mousefx::windows
