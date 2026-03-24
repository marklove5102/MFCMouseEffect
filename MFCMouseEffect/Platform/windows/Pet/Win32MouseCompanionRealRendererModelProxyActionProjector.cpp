#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelProxyActionProjector.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"

#include <algorithm>

namespace mousefx::windows {
namespace {

float ResolveNodeActionBoost(
    const Win32MouseCompanionRealRendererScene& scene,
    const std::string& logicalNode) {
    float boost = 0.0f;
    if (logicalNode == "head") {
        if (scene.actionOverlay.clickRingVisible) {
            boost += 1.0f;
        }
        if (scene.actionOverlay.scrollArcVisible) {
            boost += 0.58f;
        }
    } else if (logicalNode == "appendage") {
        if (scene.actionOverlay.holdBandVisible) {
            boost += 0.86f;
        }
        if (scene.actionOverlay.dragLineVisible) {
            boost += 0.76f;
        }
        if (scene.actionOverlay.followTrailVisible) {
            boost += 0.40f;
        }
    } else if (logicalNode == "overlay") {
        if (scene.actionOverlay.clickRingVisible) {
            boost += 0.62f;
        }
        if (scene.actionOverlay.scrollArcVisible) {
            boost += 0.88f;
        }
        if (scene.actionOverlay.dragLineVisible) {
            boost += 0.56f;
        }
        if (scene.actionOverlay.followTrailVisible) {
            boost += 0.72f;
        }
    } else if (logicalNode == "grounding") {
        if (scene.actionOverlay.holdBandVisible) {
            boost += 0.42f;
        }
        if (scene.actionOverlay.scrollArcVisible) {
            boost += 0.36f;
        }
    } else {
        if (scene.actionOverlay.holdBandVisible) {
            boost += 0.58f;
        }
        if (scene.actionOverlay.dragLineVisible) {
            boost += 0.44f;
        }
        if (scene.actionOverlay.followTrailVisible) {
            boost += 0.30f;
        }
        if (scene.actionOverlay.clickRingVisible) {
            boost += 0.18f;
        }
    }
    return std::clamp(boost, 0.0f, 1.8f);
}

float ResolveSurfaceActionBoost(
    const Win32MouseCompanionRealRendererScene& scene,
    const std::string& surfaceKey) {
    float boost = 0.0f;
    if (surfaceKey == "head_bridge") {
        if (scene.actionOverlay.clickRingVisible) {
            boost += 1.0f;
        }
        if (scene.actionOverlay.scrollArcVisible) {
            boost += 0.54f;
        }
    } else if (surfaceKey == "appendage_bridge") {
        if (scene.actionOverlay.holdBandVisible) {
            boost += 0.94f;
        }
        if (scene.actionOverlay.dragLineVisible) {
            boost += 0.78f;
        }
        if (scene.actionOverlay.followTrailVisible) {
            boost += 0.44f;
        }
    } else if (surfaceKey == "overlay_bridge") {
        if (scene.actionOverlay.scrollArcVisible) {
            boost += 0.92f;
        }
        if (scene.actionOverlay.followTrailVisible) {
            boost += 0.72f;
        }
        if (scene.actionOverlay.dragLineVisible) {
            boost += 0.46f;
        }
    } else if (surfaceKey == "grounding_bridge") {
        if (scene.actionOverlay.holdBandVisible) {
            boost += 0.56f;
        }
        if (scene.actionOverlay.scrollArcVisible) {
            boost += 0.38f;
        }
    } else if (surfaceKey == "core_shell") {
        if (scene.actionOverlay.clickRingVisible) {
            boost += 0.28f;
        }
        if (scene.actionOverlay.holdBandVisible) {
            boost += 0.46f;
        }
        if (scene.actionOverlay.dragLineVisible) {
            boost += 0.36f;
        }
        if (scene.actionOverlay.followTrailVisible) {
            boost += 0.34f;
        }
        if (scene.actionOverlay.scrollArcVisible) {
            boost += 0.42f;
        }
    }
    return std::clamp(boost, 0.0f, 1.8f);
}

void ScaleRectAboutCenter(Gdiplus::RectF* rect, float scale) {
    if (rect == nullptr || scale <= 0.0f) {
        return;
    }
    const float centerX = rect->X + rect->Width * 0.5f;
    const float centerY = rect->Y + rect->Height * 0.5f;
    rect->Width *= scale;
    rect->Height *= scale;
    rect->X = centerX - rect->Width * 0.5f;
    rect->Y = centerY - rect->Height * 0.5f;
}

void ScalePolygonAboutCentroid(std::vector<Gdiplus::PointF>* polygon, float scale) {
    if (polygon == nullptr || polygon->empty() || scale <= 0.0f) {
        return;
    }
    float centerX = 0.0f;
    float centerY = 0.0f;
    for (const auto& point : *polygon) {
        centerX += point.X;
        centerY += point.Y;
    }
    centerX /= static_cast<float>(polygon->size());
    centerY /= static_cast<float>(polygon->size());
    for (auto& point : *polygon) {
        point.X = centerX + (point.X - centerX) * scale;
        point.Y = centerY + (point.Y - centerY) * scale;
    }
}

} // namespace

void ApplyWin32MouseCompanionRealRendererModelProxyActionProjector(
    Win32MouseCompanionRealRendererScene& scene) {
    if (!scene.modelProxyVisible) {
        return;
    }

    const float dominanceMix = 0.58f + scene.proxyDominance * 0.42f;

    for (auto& silhouette : scene.modelProxySilhouettes) {
        const float boost = ResolveNodeActionBoost(scene, silhouette.logicalNode);
        if (boost <= 0.0f) {
            continue;
        }
        silhouette.alpha = std::clamp(
            silhouette.alpha + 44.0f * boost * dominanceMix,
            72.0f,
            248.0f);
        ScaleRectAboutCenter(
            &silhouette.bounds,
            1.0f + 0.046f * std::min(1.6f, boost) * dominanceMix);
    }

    for (auto& surface : scene.modelProxySurfaces) {
        const float boost = ResolveSurfaceActionBoost(scene, surface.surfaceKey);
        if (boost <= 0.0f) {
            continue;
        }
        surface.alpha = std::clamp(
            surface.alpha + 40.0f * boost * dominanceMix,
            78.0f,
            238.0f);
        ScalePolygonAboutCentroid(
            &surface.polygon,
            1.0f + 0.032f * std::min(1.5f, boost) * dominanceMix);
    }

    for (auto& link : scene.modelProxyLinks) {
        const float boost = ResolveNodeActionBoost(scene, link.logicalNode);
        if (boost <= 0.0f) {
            continue;
        }
        link.alpha = std::clamp(
            link.alpha + 34.0f * boost * dominanceMix,
            76.0f,
            236.0f);
    }

    for (auto& node : scene.modelProxyNodes) {
        const float boost = ResolveNodeActionBoost(scene, node.logicalNode);
        if (boost <= 0.0f) {
            continue;
        }
        node.alpha = std::clamp(
            node.alpha + 52.0f * boost * dominanceMix,
            92.0f,
            252.0f);
        ScaleRectAboutCenter(
            &node.bounds,
            1.0f + 0.058f * std::min(1.6f, boost) * dominanceMix);
    }
}

} // namespace mousefx::windows
