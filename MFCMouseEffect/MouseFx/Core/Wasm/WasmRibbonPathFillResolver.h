#pragma once

#include "MouseFx/Core/Wasm/WasmPathFillCommandConfig.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace mousefx::wasm {

struct ResolvedRibbonPointInput final {
    ScreenPoint center{};
    float widthPx = 12.0f;
};

inline bool TryResolveRibbonPathFillGeometry(
    const std::vector<ResolvedRibbonPointInput>& points,
    bool closed,
    float alpha,
    float glowWidthPx,
    uint32_t delayMs,
    uint32_t lifeMs,
    uint32_t fillColorArgb,
    uint32_t glowColorArgb,
    const RenderSemantics& semantics,
    ResolvedSpawnPathFillCommand* outResolved,
    std::string* outError) {
    if (!outResolved) {
        if (outError) {
            *outError = "ribbon trail output is null";
        }
        return false;
    }
    if (points.size() < 2u) {
        if (outError) {
            *outError = "ribbon trail requires at least 2 points";
        }
        return false;
    }

    std::vector<ScreenPoint> leftPoints{};
    std::vector<ScreenPoint> rightPoints{};
    leftPoints.reserve(points.size());
    rightPoints.reserve(points.size());

    auto resolveTangent = [&](size_t index) -> std::pair<double, double> {
        const size_t count = points.size();
        auto diff = [&](size_t from, size_t to) -> std::pair<double, double> {
            return {
                static_cast<double>(points[to].center.x - points[from].center.x),
                static_cast<double>(points[to].center.y - points[from].center.y),
            };
        };

        std::pair<double, double> tangent{0.0, 0.0};
        if (closed) {
            const size_t prev = (index + count - 1u) % count;
            const size_t next = (index + 1u) % count;
            tangent = diff(prev, next);
        } else if (index == 0u) {
            tangent = diff(0u, 1u);
        } else if (index + 1u == count) {
            tangent = diff(count - 2u, count - 1u);
        } else {
            tangent = diff(index - 1u, index + 1u);
        }

        const double length = std::hypot(tangent.first, tangent.second);
        if (length <= 0.001) {
            return {1.0, 0.0};
        }
        return {tangent.first / length, tangent.second / length};
    };

    for (size_t index = 0; index < points.size(); ++index) {
        const auto tangent = resolveTangent(index);
        const double nx = -tangent.second;
        const double ny = tangent.first;
        const double halfWidth = static_cast<double>(ClampPathCommandFloat(points[index].widthPx, 12.0f, 1.0f, 240.0f)) * 0.5;
        const double cx = static_cast<double>(points[index].center.x);
        const double cy = static_cast<double>(points[index].center.y);
        leftPoints.push_back(ScreenPoint{
            static_cast<int32_t>(std::lround(cx + nx * halfWidth)),
            static_cast<int32_t>(std::lround(cy + ny * halfWidth)),
        });
        rightPoints.push_back(ScreenPoint{
            static_cast<int32_t>(std::lround(cx - nx * halfWidth)),
            static_cast<int32_t>(std::lround(cy - ny * halfWidth)),
        });
    }

    std::vector<ScreenPoint> polygonPoints{};
    polygonPoints.reserve(leftPoints.size() + rightPoints.size());
    polygonPoints.insert(polygonPoints.end(), leftPoints.begin(), leftPoints.end());
    for (size_t index = rightPoints.size(); index > 0; --index) {
        polygonPoints.push_back(rightPoints[index - 1u]);
    }
    if (polygonPoints.size() < 4u) {
        if (outError) {
            *outError = "ribbon trail resolved empty polygon";
        }
        return false;
    }

    int overlayMinX = 0;
    int overlayMaxX = 0;
    int overlayMinY = 0;
    int overlayMaxY = 0;
    float screenMinX = 0.0f;
    float screenMaxX = 0.0f;
    float screenMinY = 0.0f;
    float screenMaxY = 0.0f;
    bool haveBounds = false;
    for (const ScreenPoint& point : polygonPoints) {
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(point);
        const float sx = static_cast<float>(point.x);
        const float sy = static_cast<float>(point.y);
        if (!haveBounds) {
            overlayMinX = overlayMaxX = overlayPoint.x;
            overlayMinY = overlayMaxY = overlayPoint.y;
            screenMinX = screenMaxX = sx;
            screenMinY = screenMaxY = sy;
            haveBounds = true;
        } else {
            overlayMinX = std::min(overlayMinX, overlayPoint.x);
            overlayMaxX = std::max(overlayMaxX, overlayPoint.x);
            overlayMinY = std::min(overlayMinY, overlayPoint.y);
            overlayMaxY = std::max(overlayMaxY, overlayPoint.y);
            screenMinX = std::min(screenMinX, sx);
            screenMaxX = std::max(screenMaxX, sx);
            screenMinY = std::min(screenMinY, sy);
            screenMaxY = std::max(screenMaxY, sy);
        }
    }
    if (!haveBounds) {
        if (outError) {
            *outError = "ribbon trail resolved empty bounds";
        }
        return false;
    }

    ResolvedSpawnPathFillCommand resolved{};
    resolved.alpha = ClampPathCommandFloat(alpha, 1.0f, 0.0f, 1.0f);
    resolved.glowWidthPx = ClampPathCommandFloat(glowWidthPx, 8.0f, 0.0f, 64.0f);
    resolved.delayMs = delayMs;
    resolved.lifeMs = lifeMs;
    resolved.fillRule = kPathFillRuleNonZero;
    resolved.semantics = semantics;
    resolved.fillColorArgb = fillColorArgb;
    resolved.glowColorArgb = glowColorArgb;
    resolved.fillArgb = ScalePathCommandArgb(fillColorArgb, resolved.alpha);
    resolved.glowArgb = ScalePathCommandArgb(glowColorArgb, resolved.alpha);

    const int paddingPx = std::max(14, static_cast<int>(std::ceil(resolved.glowWidthPx * 2.5f)));
    overlayMinX -= paddingPx;
    overlayMaxX += paddingPx;
    overlayMinY -= paddingPx;
    overlayMaxY += paddingPx;

    const int widthPx = std::max(1, overlayMaxX - overlayMinX);
    const int heightPx = std::max(1, overlayMaxY - overlayMinY);
    const int sidePx = std::clamp(std::max(widthPx, heightPx), 32, 1536);
    const int offsetX = (sidePx - widthPx) / 2;
    const int offsetY = (sidePx - heightPx) / 2;
    resolved.frameLeftPx = overlayMinX - offsetX;
    resolved.frameTopPx = overlayMinY - offsetY;
    resolved.squareSizePx = sidePx;
    resolved.centerScreenPt = ScreenPoint{
        static_cast<int32_t>(std::lround((screenMinX + screenMaxX) * 0.5f)),
        static_cast<int32_t>(std::lround((screenMinY + screenMaxY) * 0.5f)),
    };

    resolved.localNodes.reserve(polygonPoints.size() + 1u);
    for (size_t index = 0; index < polygonPoints.size(); ++index) {
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(polygonPoints[index]);
        resolved.localNodes.push_back(ResolvedPathNode{
            (index == 0u) ? kPathStrokeNodeOpMoveTo : kPathStrokeNodeOpLineTo,
            static_cast<float>(overlayPoint.x - resolved.frameLeftPx),
            static_cast<float>(overlayPoint.y - resolved.frameTopPx),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
        });
    }
    resolved.localNodes.push_back(ResolvedPathNode{
        kPathStrokeNodeOpClose,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
    });

    *outResolved = std::move(resolved);
    return true;
}

} // namespace mousefx::wasm
