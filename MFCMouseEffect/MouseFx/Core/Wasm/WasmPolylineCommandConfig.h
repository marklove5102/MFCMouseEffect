#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace mousefx::wasm {

constexpr uint16_t kMaxSpawnPolylinePoints = 96u;

struct ResolvedSpawnPolylineCommand final {
    ScreenPoint centerScreenPt{};
    int frameLeftPx = 0;
    int frameTopPx = 0;
    int squareSizePx = 64;
    std::vector<float> localPointsXY{};
    float lineWidthPx = 4.0f;
    float alpha = 1.0f;
    uint32_t delayMs = 0;
    uint32_t lifeMs = 320;
    uint32_t strokeColorArgb = 0xFFFFFFFFu;
    uint32_t glowColorArgb = 0x66FFFFFFu;
    uint32_t strokeArgb = 0xFFFFFFFFu;
    uint32_t glowArgb = 0x66FFFFFFu;
    bool closed = false;
};

inline float ClampSpawnPolylineFloat(float value, float fallback, float minValue, float maxValue) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

inline uint32_t ResolveSpawnPolylineLifeMs(uint32_t lifeMs, uint32_t fallbackLifeMs) {
    const uint32_t base = (lifeMs > 0u) ? lifeMs : fallbackLifeMs;
    return std::clamp<uint32_t>(base, 40u, 8000u);
}

inline uint32_t ResolveSpawnPolylineDelayMs(uint32_t delayMs) {
    return std::min<uint32_t>(delayMs, 60000u);
}

inline uint32_t ScaleSpawnPolylineArgb(uint32_t argb, float alphaScale) {
    const float clampedAlpha = std::clamp(alphaScale, 0.0f, 1.0f);
    const uint32_t baseAlpha = (argb >> 24) & 0xFFu;
    const uint32_t scaledAlpha = static_cast<uint32_t>(
        std::lround(static_cast<double>(baseAlpha) * static_cast<double>(clampedAlpha)));
    return (argb & 0x00FFFFFFu) | ((scaledAlpha & 0xFFu) << 24);
}

inline uint32_t ResolveSpawnPolylineGlowColor(uint32_t strokeArgb, uint32_t glowArgb) {
    if (glowArgb != 0u) {
        return glowArgb;
    }
    return (strokeArgb & 0x00FFFFFFu) | 0x66000000u;
}

inline ScreenPoint ClampPolylinePoint(float x, float y) {
    const double clampedX = std::clamp(static_cast<double>(x), -32768.0, 32768.0);
    const double clampedY = std::clamp(static_cast<double>(y), -32768.0, 32768.0);
    return ScreenPoint{
        static_cast<int32_t>(std::lround(clampedX)),
        static_cast<int32_t>(std::lround(clampedY)),
    };
}

inline bool TryResolveSpawnPolylineCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    ResolvedSpawnPolylineCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "spawn_polyline command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(SpawnPolylineCommandV1)) {
        if (outError) {
            *outError = "spawn_polyline command truncated";
        }
        return false;
    }

    SpawnPolylineCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.pointCount < 2u) {
        if (outError) {
            *outError = "spawn_polyline requires at least 2 points";
        }
        return false;
    }
    if (cmd.pointCount > kMaxSpawnPolylinePoints) {
        if (outError) {
            *outError = "spawn_polyline point_count exceeds limit";
        }
        return false;
    }

    const size_t requiredBytes = sizeof(SpawnPolylineCommandV1) +
        static_cast<size_t>(cmd.pointCount) * sizeof(PolylinePointV1);
    if (requiredBytes > sizeBytes) {
        if (outError) {
            *outError = "spawn_polyline point payload truncated";
        }
        return false;
    }

    ResolvedSpawnPolylineCommand resolved{};
    resolved.lineWidthPx = ClampSpawnPolylineFloat(cmd.lineWidthPx, config.trail.lineWidth, 0.5f, 48.0f);
    resolved.alpha = ClampSpawnPolylineFloat(cmd.alpha, 1.0f, 0.0f, 1.0f);
    resolved.delayMs = ResolveSpawnPolylineDelayMs(cmd.delayMs);
    resolved.lifeMs = ResolveSpawnPolylineLifeMs(cmd.lifeMs, static_cast<uint32_t>(config.trail.durationMs));
    resolved.closed = (cmd.flags & kSpawnPolylineFlagClosed) != 0u;

    const uint32_t themeStrokeArgb = config.trail.color.value;
    resolved.strokeColorArgb = (cmd.strokeArgb != 0u) ? cmd.strokeArgb : themeStrokeArgb;
    resolved.glowColorArgb = ResolveSpawnPolylineGlowColor(resolved.strokeColorArgb, cmd.glowArgb);
    resolved.strokeArgb = ScaleSpawnPolylineArgb(resolved.strokeColorArgb, resolved.alpha);
    resolved.glowArgb = ScaleSpawnPolylineArgb(resolved.glowColorArgb, resolved.alpha);

    float screenMinX = 0.0f;
    float screenMaxX = 0.0f;
    float screenMinY = 0.0f;
    float screenMaxY = 0.0f;
    int overlayMinX = 0;
    int overlayMaxX = 0;
    int overlayMinY = 0;
    int overlayMaxY = 0;
    bool havePoint = false;

    const size_t pointsOffset = sizeof(SpawnPolylineCommandV1);
    std::vector<ScreenPoint> overlayPoints;
    overlayPoints.reserve(cmd.pointCount);
    resolved.localPointsXY.reserve(static_cast<size_t>(cmd.pointCount) * 2u);

    for (uint16_t index = 0; index < cmd.pointCount; ++index) {
        PolylinePointV1 point{};
        std::memcpy(
            &point,
            raw + pointsOffset + static_cast<size_t>(index) * sizeof(PolylinePointV1),
            sizeof(point));
        const ScreenPoint screenPoint = ClampPolylinePoint(point.x, point.y);
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(screenPoint);
        overlayPoints.push_back(overlayPoint);

        const float screenX = static_cast<float>(screenPoint.x);
        const float screenY = static_cast<float>(screenPoint.y);
        if (!havePoint) {
            screenMinX = screenMaxX = screenX;
            screenMinY = screenMaxY = screenY;
            overlayMinX = overlayMaxX = overlayPoint.x;
            overlayMinY = overlayMaxY = overlayPoint.y;
            havePoint = true;
        } else {
            screenMinX = std::min(screenMinX, screenX);
            screenMaxX = std::max(screenMaxX, screenX);
            screenMinY = std::min(screenMinY, screenY);
            screenMaxY = std::max(screenMaxY, screenY);
            overlayMinX = std::min(overlayMinX, overlayPoint.x);
            overlayMaxX = std::max(overlayMaxX, overlayPoint.x);
            overlayMinY = std::min(overlayMinY, overlayPoint.y);
            overlayMaxY = std::max(overlayMaxY, overlayPoint.y);
        }
    }

    if (!havePoint) {
        if (outError) {
            *outError = "spawn_polyline resolved empty point set";
        }
        return false;
    }

    const int paddingPx = std::max(12, static_cast<int>(std::ceil(resolved.lineWidthPx * 4.0f)));
    const int widthPx = std::max(1, overlayMaxX - overlayMinX);
    const int heightPx = std::max(1, overlayMaxY - overlayMinY);
    const int sidePx = std::clamp(std::max(widthPx, heightPx) + paddingPx * 2, 32, 1024);
    const int offsetX = (sidePx - widthPx) / 2;
    const int offsetY = (sidePx - heightPx) / 2;
    resolved.frameLeftPx = overlayMinX - offsetX;
    resolved.frameTopPx = overlayMinY - offsetY;
    resolved.squareSizePx = sidePx;

    for (const ScreenPoint& overlayPoint : overlayPoints) {
        resolved.localPointsXY.push_back(static_cast<float>(overlayPoint.x - resolved.frameLeftPx));
        resolved.localPointsXY.push_back(static_cast<float>(overlayPoint.y - resolved.frameTopPx));
    }

    resolved.centerScreenPt.x = static_cast<int32_t>(std::lround((screenMinX + screenMaxX) * 0.5f));
    resolved.centerScreenPt.y = static_cast<int32_t>(std::lround((screenMinY + screenMaxY) * 0.5f));
    *outResolved = std::move(resolved);
    return true;
}

} // namespace mousefx::wasm
