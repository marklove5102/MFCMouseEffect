#pragma once

#include "MouseFx/Core/Wasm/WasmSpriteBatchCommandConfig.h"

namespace mousefx::wasm {

constexpr uint16_t kMaxSpawnQuadBatchItems = 128u;

inline float ResolveSpawnQuadBatchExtentPx(float value, float fallback) {
    return ClampSpawnSpriteBatchFloat(value, fallback, 2.0f, 640.0f);
}

inline float ResolveQuadBatchUv(float value, float fallback) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, 0.0f, 1.0f);
}

inline bool TryResolveSpawnQuadBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    bool overlayMotionYUp,
    ResolvedSpawnSpriteBatchCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "spawn_quad_batch command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(SpawnQuadBatchCommandV1)) {
        if (outError) {
            *outError = "spawn_quad_batch command truncated";
        }
        return false;
    }

    SpawnQuadBatchCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.itemCount == 0u) {
        if (outError) {
            *outError = "spawn_quad_batch requires at least 1 item";
        }
        return false;
    }
    if (cmd.itemCount > kMaxSpawnQuadBatchItems) {
        if (outError) {
            *outError = "spawn_quad_batch item_count exceeds limit";
        }
        return false;
    }

    const size_t requiredBytes = sizeof(SpawnQuadBatchCommandV1) +
        static_cast<size_t>(cmd.itemCount) * sizeof(QuadBatchItemV1);
    if (requiredBytes > sizeBytes) {
        if (outError) {
            *outError = "spawn_quad_batch item payload truncated";
        }
        return false;
    }

    ResolvedSpawnSpriteBatchCommand resolved{};
    resolved.delayMs = ResolveSpawnImageDelayMs(cmd.delayMs);
    resolved.lifeMs = ResolveSpawnImageLifeMs(cmd.lifeMs, config.icon.durationMs);
    const bool legacyScreenBlend = (cmd.flags & kSpawnQuadBatchFlagScreenBlend) != 0u;
    if (!TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
            raw,
            sizeBytes,
            requiredBytes,
            legacyScreenBlend,
            &resolved.semantics,
            outError,
            "spawn_quad_batch")) {
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

    const size_t itemsOffset = sizeof(SpawnQuadBatchCommandV1);
    for (uint16_t index = 0; index < cmd.itemCount; ++index) {
        QuadBatchItemV1 item{};
        std::memcpy(
            &item,
            raw + itemsOffset + static_cast<size_t>(index) * sizeof(QuadBatchItemV1),
            sizeof(item));

        const ScreenPoint screenPoint = ClampSpriteBatchScreenPoint(item.x, item.y);
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(screenPoint);
        const float widthPx = ResolveSpawnQuadBatchExtentPx(item.widthPx, 64.0f);
        const float heightPx = ResolveSpawnQuadBatchExtentPx(item.heightPx, widthPx);
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

        float srcU0 = ResolveQuadBatchUv(item.srcU0, 0.0f);
        float srcV0 = ResolveQuadBatchUv(item.srcV0, 0.0f);
        float srcU1 = ResolveQuadBatchUv(item.srcU1, 1.0f);
        float srcV1 = ResolveQuadBatchUv(item.srcV1, 1.0f);
        if (srcU1 <= srcU0) {
            srcU0 = 0.0f;
            srcU1 = 1.0f;
        }
        if (srcV1 <= srcV0) {
            srcV0 = 0.0f;
            srcV1 = 1.0f;
        }

        ResolvedSpriteBatchItem resolvedItem{};
        resolvedItem.widthPx = widthPx;
        resolvedItem.heightPx = heightPx;
        resolvedItem.alpha = alpha;
        resolvedItem.rotationRad = rotationRad;
        resolvedItem.tintArgb = render_values::ResolveImageTintArgb(config.icon, item.tintArgb);
        resolvedItem.applyTint = ResolveSpawnImageApplyTint(item.tintArgb);
        resolvedItem.srcU0 = srcU0;
        resolvedItem.srcV0 = srcV0;
        resolvedItem.srcU1 = srcU1;
        resolvedItem.srcV1 = srcV1;
        resolvedItem.velocityX = ClampSpawnSpriteBatchFloat(motion.velocityX, 0.0f, -2400.0f, 2400.0f);
        resolvedItem.velocityY = ClampSpawnSpriteBatchFloat(motion.velocityY, 0.0f, -2400.0f, 2400.0f);
        resolvedItem.accelerationX = ClampSpawnSpriteBatchFloat(motion.accelerationX, 0.0f, -4800.0f, 4800.0f);
        resolvedItem.accelerationY = ClampSpawnSpriteBatchFloat(motion.accelerationY, 0.0f, -4800.0f, 4800.0f);
        WasmPluginImageAssetCatalog::ResolveImageAssetPath(activeManifestPath, item.imageId, &resolvedItem.assetPath, nullptr);

        const double halfWidth = static_cast<double>(widthPx) * 0.5 + 6.0;
        const double halfHeight = static_cast<double>(heightPx) * 0.5 + 6.0;
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
            *outError = "spawn_quad_batch resolved empty item set";
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

    resolved.centerScreenPt = ScreenPoint{
        static_cast<int32_t>(std::lround((screenMinX + screenMaxX) * 0.5)),
        static_cast<int32_t>(std::lround((screenMinY + screenMaxY) * 0.5)),
    };
    *outResolved = std::move(resolved);
    return true;
}

} // namespace mousefx::wasm
