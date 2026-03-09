#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Core/Wasm/WasmCommandCoordinateSemantics.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
#include "MouseFx/Core/Wasm/WasmImageRuntimeConfig.h"
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

constexpr uint16_t kMaxSpawnGlowBatchItems = 128u;

struct ResolvedGlowBatchItem final {
    float localX = 0.0f;
    float localY = 0.0f;
    float radiusPx = 6.0f;
    float alpha = 1.0f;
    uint32_t colorArgb = 0xFFFFFFFFu;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float accelerationX = 0.0f;
    float accelerationY = 0.0f;
};

struct ResolvedSpawnGlowBatchCommand final {
    ScreenPoint centerScreenPt{};
    int frameLeftPx = 0;
    int frameTopPx = 0;
    int squareSizePx = 64;
    std::vector<ResolvedGlowBatchItem> items{};
    uint32_t delayMs = 0;
    uint32_t lifeMs = 360;
    RenderSemantics semantics{};
};

inline float ClampSpawnGlowBatchFloat(float value, float fallback, float minValue, float maxValue) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

inline uint32_t ResolveSpawnGlowBatchLifeMs(uint32_t lifeMs, int configDurationMs) {
    if (lifeMs > 0u) {
        return std::clamp<uint32_t>(lifeMs, 60u, 10000u);
    }
    const int fallback = std::max(60, configDurationMs);
    return std::clamp<uint32_t>(static_cast<uint32_t>(fallback), 60u, 10000u);
}

inline uint32_t ResolveSpawnGlowBatchDelayMs(uint32_t delayMs) {
    return std::clamp<uint32_t>(delayMs, 0u, 60000u);
}

inline ScreenPoint ClampGlowBatchScreenPoint(float x, float y) {
    const double clampedX = std::clamp(static_cast<double>(x), -32768.0, 32768.0);
    const double clampedY = std::clamp(static_cast<double>(y), -32768.0, 32768.0);
    return ScreenPoint{
        static_cast<int32_t>(std::lround(clampedX)),
        static_cast<int32_t>(std::lround(clampedY)),
    };
}

inline bool TryResolveSpawnGlowBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    bool overlayMotionYUp,
    ResolvedSpawnGlowBatchCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "spawn_glow_batch command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(SpawnGlowBatchCommandV1)) {
        if (outError) {
            *outError = "spawn_glow_batch command truncated";
        }
        return false;
    }

    SpawnGlowBatchCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.itemCount == 0u) {
        if (outError) {
            *outError = "spawn_glow_batch requires at least 1 item";
        }
        return false;
    }
    if (cmd.itemCount > kMaxSpawnGlowBatchItems) {
        if (outError) {
            *outError = "spawn_glow_batch item_count exceeds limit";
        }
        return false;
    }

    const size_t requiredBytes = sizeof(SpawnGlowBatchCommandV1) +
        static_cast<size_t>(cmd.itemCount) * sizeof(GlowBatchItemV1);
    if (requiredBytes > sizeBytes) {
        if (outError) {
            *outError = "spawn_glow_batch item payload truncated";
        }
        return false;
    }

    ResolvedSpawnGlowBatchCommand resolved{};
    resolved.delayMs = ResolveSpawnGlowBatchDelayMs(cmd.delayMs);
    resolved.lifeMs = ResolveSpawnGlowBatchLifeMs(cmd.lifeMs, config.icon.durationMs);
    const bool legacyScreenBlend = (cmd.flags & kSpawnGlowBatchFlagScreenBlend) != 0u;
    if (!TryResolveOptionalCommandRenderSemanticsTail(
            raw,
            sizeBytes,
            requiredBytes,
            legacyScreenBlend,
            &resolved.semantics,
            outError,
            "spawn_glow_batch")) {
        return false;
    }

    const double lifeSec = static_cast<double>(resolved.lifeMs) / 1000.0;
    const uint32_t fallbackColor = config.icon.fillColor.value;

    double screenMinX = 0.0;
    double screenMaxX = 0.0;
    double screenMinY = 0.0;
    double screenMaxY = 0.0;
    double overlayMinX = 0.0;
    double overlayMaxX = 0.0;
    double overlayMinY = 0.0;
    double overlayMaxY = 0.0;
    bool haveBounds = false;

    const size_t itemsOffset = sizeof(SpawnGlowBatchCommandV1);
    struct OverlayItemSeed final {
        ScreenPoint overlayPoint{};
        ResolvedGlowBatchItem item{};
    };
    std::vector<OverlayItemSeed> overlayItems{};
    overlayItems.reserve(cmd.itemCount);

    for (uint16_t index = 0; index < cmd.itemCount; ++index) {
        GlowBatchItemV1 item{};
        std::memcpy(
            &item,
            raw + itemsOffset + static_cast<size_t>(index) * sizeof(GlowBatchItemV1),
            sizeof(item));

        const ScreenPoint screenPoint = ClampGlowBatchScreenPoint(item.x, item.y);
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(screenPoint);
        const float radiusPx = ClampSpawnGlowBatchFloat(item.radiusPx, 6.0f, 1.0f, 96.0f);
        const float alpha = std::isfinite(item.alpha) ? std::clamp(item.alpha, 0.0f, 1.0f) : 1.0f;

        WasmCommandMotion motion{
            item.vx,
            item.vy,
            item.ax,
            item.ay,
        };
        if (overlayMotionYUp) {
            motion = ConvertMotionToOverlayYUp(motion);
        }

        ResolvedGlowBatchItem resolvedItem{};
        resolvedItem.radiusPx = radiusPx;
        resolvedItem.alpha = alpha;
        resolvedItem.colorArgb = (item.colorArgb != 0u) ? item.colorArgb : fallbackColor;
        resolvedItem.velocityX = ClampSpawnGlowBatchFloat(motion.velocityX, 0.0f, -2400.0f, 2400.0f);
        resolvedItem.velocityY = ClampSpawnGlowBatchFloat(motion.velocityY, 0.0f, -2400.0f, 2400.0f);
        resolvedItem.accelerationX = ClampSpawnGlowBatchFloat(motion.accelerationX, 0.0f, -4800.0f, 4800.0f);
        resolvedItem.accelerationY = ClampSpawnGlowBatchFloat(motion.accelerationY, 0.0f, -4800.0f, 4800.0f);

        const double overlayStartX = static_cast<double>(overlayPoint.x);
        const double overlayStartY = static_cast<double>(overlayPoint.y);
        const double overlayEndX = overlayStartX +
            static_cast<double>(resolvedItem.velocityX) * lifeSec +
            0.5 * static_cast<double>(resolvedItem.accelerationX) * lifeSec * lifeSec;
        const double overlayEndY = overlayStartY +
            static_cast<double>(resolvedItem.velocityY) * lifeSec +
            0.5 * static_cast<double>(resolvedItem.accelerationY) * lifeSec * lifeSec;
        const double glowExtent = static_cast<double>(radiusPx) * 3.2 + 6.0;

        const double screenStartX = static_cast<double>(screenPoint.x);
        const double screenStartY = static_cast<double>(screenPoint.y);
        const double screenEndX = screenStartX +
            static_cast<double>(item.vx) * lifeSec +
            0.5 * static_cast<double>(item.ax) * lifeSec * lifeSec;
        const double screenEndY = screenStartY +
            static_cast<double>(item.vy) * lifeSec +
            0.5 * static_cast<double>(item.ay) * lifeSec * lifeSec;

        const double itemOverlayMinX = std::min(overlayStartX, overlayEndX) - glowExtent;
        const double itemOverlayMaxX = std::max(overlayStartX, overlayEndX) + glowExtent;
        const double itemOverlayMinY = std::min(overlayStartY, overlayEndY) - glowExtent;
        const double itemOverlayMaxY = std::max(overlayStartY, overlayEndY) + glowExtent;

        const double itemScreenMinX = std::min(screenStartX, screenEndX) - glowExtent;
        const double itemScreenMaxX = std::max(screenStartX, screenEndX) + glowExtent;
        const double itemScreenMinY = std::min(screenStartY, screenEndY) - glowExtent;
        const double itemScreenMaxY = std::max(screenStartY, screenEndY) + glowExtent;

        if (!haveBounds) {
            overlayMinX = itemOverlayMinX;
            overlayMaxX = itemOverlayMaxX;
            overlayMinY = itemOverlayMinY;
            overlayMaxY = itemOverlayMaxY;
            screenMinX = itemScreenMinX;
            screenMaxX = itemScreenMaxX;
            screenMinY = itemScreenMinY;
            screenMaxY = itemScreenMaxY;
            haveBounds = true;
        } else {
            overlayMinX = std::min(overlayMinX, itemOverlayMinX);
            overlayMaxX = std::max(overlayMaxX, itemOverlayMaxX);
            overlayMinY = std::min(overlayMinY, itemOverlayMinY);
            overlayMaxY = std::max(overlayMaxY, itemOverlayMaxY);
            screenMinX = std::min(screenMinX, itemScreenMinX);
            screenMaxX = std::max(screenMaxX, itemScreenMaxX);
            screenMinY = std::min(screenMinY, itemScreenMinY);
            screenMaxY = std::max(screenMaxY, itemScreenMaxY);
        }

        overlayItems.push_back(OverlayItemSeed{
            overlayPoint,
            resolvedItem,
        });
    }

    if (!haveBounds) {
        if (outError) {
            *outError = "spawn_glow_batch resolved empty item set";
        }
        return false;
    }

    const int widthPx = std::max(1, static_cast<int>(std::ceil(overlayMaxX - overlayMinX)));
    const int heightPx = std::max(1, static_cast<int>(std::ceil(overlayMaxY - overlayMinY)));
    const int sidePx = std::clamp(std::max(widthPx, heightPx), 32, 1536);
    const int frameLeftPx = static_cast<int>(std::floor(overlayMinX));
    const int frameTopPx = static_cast<int>(std::floor(overlayMinY));
    const int offsetX = (sidePx - widthPx) / 2;
    const int offsetY = (sidePx - heightPx) / 2;
    resolved.frameLeftPx = frameLeftPx - offsetX;
    resolved.frameTopPx = frameTopPx - offsetY;
    resolved.squareSizePx = sidePx;

    resolved.items.reserve(overlayItems.size());
    for (const OverlayItemSeed& overlayItem : overlayItems) {
        ResolvedGlowBatchItem item = overlayItem.item;
        item.localX = static_cast<float>(overlayItem.overlayPoint.x - resolved.frameLeftPx);
        item.localY = static_cast<float>(overlayItem.overlayPoint.y - resolved.frameTopPx);
        resolved.items.push_back(item);
    }

    resolved.centerScreenPt.x = static_cast<int32_t>(std::lround((screenMinX + screenMaxX) * 0.5));
    resolved.centerScreenPt.y = static_cast<int32_t>(std::lround((screenMinY + screenMaxY) * 0.5));
    *outResolved = std::move(resolved);
    return true;
}

} // namespace mousefx::wasm
