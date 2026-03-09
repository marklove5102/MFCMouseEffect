#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Core/Wasm/WasmCommandCoordinateSemantics.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
#include "MouseFx/Core/Wasm/WasmImageRuntimeConfig.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"
#include "MouseFx/Core/Wasm/WasmPluginImageAssetCatalog.h"
#include "MouseFx/Core/Wasm/WasmRenderValueResolver.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace mousefx::wasm {

constexpr uint16_t kMaxSpawnSpriteBatchItems = 128u;

struct ResolvedSpriteBatchItem final {
    std::wstring assetPath{};
    float localX = 0.0f;
    float localY = 0.0f;
    float widthPx = 24.0f;
    float heightPx = 24.0f;
    float alpha = 1.0f;
    float rotationRad = 0.0f;
    uint32_t tintArgb = 0xFFFFFFFFu;
    bool applyTint = false;
    float srcU0 = 0.0f;
    float srcV0 = 0.0f;
    float srcU1 = 1.0f;
    float srcV1 = 1.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float accelerationX = 0.0f;
    float accelerationY = 0.0f;
};

struct ResolvedSpawnSpriteBatchCommand final {
    ScreenPoint centerScreenPt{};
    int frameLeftPx = 0;
    int frameTopPx = 0;
    int squareSizePx = 64;
    std::vector<ResolvedSpriteBatchItem> items{};
    uint32_t delayMs = 0;
    uint32_t lifeMs = 360;
    RenderSemantics semantics{};
};

inline float ClampSpawnSpriteBatchFloat(float value, float fallback, float minValue, float maxValue) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

inline float ResolveSpawnSpriteBatchSizePx(float scale) {
    const float resolvedScale = ResolveSpawnImageScale(scale);
    return ClampSpawnSpriteBatchFloat(120.0f * resolvedScale, 120.0f, 8.0f, 420.0f);
}

inline ScreenPoint ClampSpriteBatchScreenPoint(float x, float y) {
    const double clampedX = std::clamp(static_cast<double>(x), -32768.0, 32768.0);
    const double clampedY = std::clamp(static_cast<double>(y), -32768.0, 32768.0);
    return ScreenPoint{
        static_cast<int32_t>(std::lround(clampedX)),
        static_cast<int32_t>(std::lround(clampedY)),
    };
}

inline bool TryResolveSpawnSpriteBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    bool overlayMotionYUp,
    ResolvedSpawnSpriteBatchCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "spawn_sprite_batch command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(SpawnSpriteBatchCommandV1)) {
        if (outError) {
            *outError = "spawn_sprite_batch command truncated";
        }
        return false;
    }

    SpawnSpriteBatchCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.itemCount == 0u) {
        if (outError) {
            *outError = "spawn_sprite_batch requires at least 1 item";
        }
        return false;
    }
    if (cmd.itemCount > kMaxSpawnSpriteBatchItems) {
        if (outError) {
            *outError = "spawn_sprite_batch item_count exceeds limit";
        }
        return false;
    }

    const size_t requiredBytes = sizeof(SpawnSpriteBatchCommandV1) +
        static_cast<size_t>(cmd.itemCount) * sizeof(SpriteBatchItemV1);
    if (requiredBytes > sizeBytes) {
        if (outError) {
            *outError = "spawn_sprite_batch item payload truncated";
        }
        return false;
    }

    ResolvedSpawnSpriteBatchCommand resolved{};
    resolved.delayMs = ResolveSpawnImageDelayMs(cmd.delayMs);
    resolved.lifeMs = ResolveSpawnImageLifeMs(cmd.lifeMs, config.icon.durationMs);
    const bool legacyScreenBlend = (cmd.flags & kSpawnSpriteBatchFlagScreenBlend) != 0u;
    if (!TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
            raw,
            sizeBytes,
            requiredBytes,
            legacyScreenBlend,
            &resolved.semantics,
            outError,
            "spawn_sprite_batch")) {
        return false;
    }

    const double lifeSec = static_cast<double>(resolved.lifeMs) / 1000.0;
    double screenMinX = 0.0;
    double screenMaxX = 0.0;
    double screenMinY = 0.0;
    double screenMaxY = 0.0;
    double overlayMinX = 0.0;
    double overlayMaxX = 0.0;
    double overlayMinY = 0.0;
    double overlayMaxY = 0.0;
    bool haveBounds = false;

    struct OverlayItemSeed final {
        ScreenPoint overlayPoint{};
        ResolvedSpriteBatchItem item{};
    };
    std::vector<OverlayItemSeed> overlayItems{};
    overlayItems.reserve(cmd.itemCount);

    const size_t itemsOffset = sizeof(SpawnSpriteBatchCommandV1);
    for (uint16_t index = 0; index < cmd.itemCount; ++index) {
        SpriteBatchItemV1 item{};
        std::memcpy(
            &item,
            raw + itemsOffset + static_cast<size_t>(index) * sizeof(SpriteBatchItemV1),
            sizeof(item));

        const ScreenPoint screenPoint = ClampSpriteBatchScreenPoint(item.x, item.y);
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(screenPoint);
        const float sizePx = ResolveSpawnSpriteBatchSizePx(item.scale);
        const float alpha = ResolveSpawnImageAlpha(item.alpha);
        const float rotationRad = std::isfinite(item.rotation) ? item.rotation : 0.0f;

        WasmCommandMotion motion{
            item.vx,
            item.vy,
            item.ax,
            item.ay,
        };
        if (overlayMotionYUp) {
            motion = ConvertMotionToOverlayYUp(motion);
        }

        ResolvedSpriteBatchItem resolvedItem{};
        resolvedItem.widthPx = sizePx;
        resolvedItem.heightPx = sizePx;
        resolvedItem.alpha = alpha;
        resolvedItem.rotationRad = rotationRad;
        resolvedItem.tintArgb = render_values::ResolveImageTintArgb(config.icon, item.tintArgb);
        resolvedItem.applyTint = ResolveSpawnImageApplyTint(item.tintArgb);
        resolvedItem.srcU0 = 0.0f;
        resolvedItem.srcV0 = 0.0f;
        resolvedItem.srcU1 = 1.0f;
        resolvedItem.srcV1 = 1.0f;
        resolvedItem.velocityX = ClampSpawnSpriteBatchFloat(motion.velocityX, 0.0f, -2400.0f, 2400.0f);
        resolvedItem.velocityY = ClampSpawnSpriteBatchFloat(motion.velocityY, 0.0f, -2400.0f, 2400.0f);
        resolvedItem.accelerationX = ClampSpawnSpriteBatchFloat(motion.accelerationX, 0.0f, -4800.0f, 4800.0f);
        resolvedItem.accelerationY = ClampSpawnSpriteBatchFloat(motion.accelerationY, 0.0f, -4800.0f, 4800.0f);
        WasmPluginImageAssetCatalog::ResolveImageAssetPath(activeManifestPath, item.imageId, &resolvedItem.assetPath, nullptr);

        const double halfWidth = static_cast<double>(resolvedItem.widthPx) * 0.5 + 6.0;
        const double halfHeight = static_cast<double>(resolvedItem.heightPx) * 0.5 + 6.0;
        const double overlayStartX = static_cast<double>(overlayPoint.x);
        const double overlayStartY = static_cast<double>(overlayPoint.y);
        const double overlayEndX = overlayStartX +
            static_cast<double>(resolvedItem.velocityX) * lifeSec +
            0.5 * static_cast<double>(resolvedItem.accelerationX) * lifeSec * lifeSec;
        const double overlayEndY = overlayStartY +
            static_cast<double>(resolvedItem.velocityY) * lifeSec +
            0.5 * static_cast<double>(resolvedItem.accelerationY) * lifeSec * lifeSec;
        const double screenStartX = static_cast<double>(screenPoint.x);
        const double screenStartY = static_cast<double>(screenPoint.y);
        const double screenEndX = screenStartX +
            static_cast<double>(item.vx) * lifeSec +
            0.5 * static_cast<double>(item.ax) * lifeSec * lifeSec;
        const double screenEndY = screenStartY +
            static_cast<double>(item.vy) * lifeSec +
            0.5 * static_cast<double>(item.ay) * lifeSec * lifeSec;

        const double itemOverlayMinX = std::min(overlayStartX, overlayEndX) - halfWidth;
        const double itemOverlayMaxX = std::max(overlayStartX, overlayEndX) + halfWidth;
        const double itemOverlayMinY = std::min(overlayStartY, overlayEndY) - halfHeight;
        const double itemOverlayMaxY = std::max(overlayStartY, overlayEndY) + halfHeight;
        const double itemScreenMinX = std::min(screenStartX, screenEndX) - halfWidth;
        const double itemScreenMaxX = std::max(screenStartX, screenEndX) + halfWidth;
        const double itemScreenMinY = std::min(screenStartY, screenEndY) - halfHeight;
        const double itemScreenMaxY = std::max(screenStartY, screenEndY) + halfHeight;

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
            *outError = "spawn_sprite_batch resolved empty item set";
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
        ResolvedSpriteBatchItem item = overlayItem.item;
        item.localX = static_cast<float>(overlayItem.overlayPoint.x - resolved.frameLeftPx);
        item.localY = static_cast<float>(overlayItem.overlayPoint.y - resolved.frameTopPx);
        resolved.items.push_back(std::move(item));
    }

    resolved.centerScreenPt.x = static_cast<int32_t>(std::lround((screenMinX + screenMaxX) * 0.5));
    resolved.centerScreenPt.y = static_cast<int32_t>(std::lround((screenMinY + screenMaxY) * 0.5));
    *outResolved = std::move(resolved);
    return true;
}

} // namespace mousefx::wasm
