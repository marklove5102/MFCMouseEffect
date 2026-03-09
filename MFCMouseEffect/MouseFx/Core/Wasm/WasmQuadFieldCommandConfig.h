#pragma once

#include "MouseFx/Core/Wasm/WasmQuadBatchCommandConfig.h"

#include <vector>

namespace mousefx::wasm {

struct ResolvedQuadFieldCommand final {
    uint32_t fieldId = 0u;
    uint32_t ttlMs = 640u;
    bool useGroupLocalOrigin = false;
    std::vector<ResolvedSpriteBatchItem> sourceItems{};
    ResolvedSpawnSpriteBatchCommand batch{};
};

inline bool TryResolveUpsertQuadFieldCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    bool overlayMotionYUp,
    ResolvedQuadFieldCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_quad_field command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertQuadFieldCommandV1)) {
        if (outError) {
            *outError = "upsert_quad_field command truncated";
        }
        return false;
    }

    UpsertQuadFieldCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.fieldId == 0u) {
        if (outError) {
            *outError = "upsert_quad_field requires non-zero field_id";
        }
        return false;
    }
    if (cmd.itemCount == 0u) {
        if (outError) {
            *outError = "upsert_quad_field requires at least 1 item";
        }
        return false;
    }
    if (cmd.itemCount > kMaxSpawnQuadBatchItems) {
        if (outError) {
            *outError = "upsert_quad_field item_count exceeds limit";
        }
        return false;
    }

    const size_t itemBytes = static_cast<size_t>(cmd.itemCount) * sizeof(QuadBatchItemV1);
    const size_t requiredBytes = sizeof(UpsertQuadFieldCommandV1) + itemBytes;
    if (requiredBytes > sizeBytes) {
        if (outError) {
            *outError = "upsert_quad_field item payload truncated";
        }
        return false;
    }

    std::vector<uint8_t> quadBytes(requiredBytes, 0u);
    SpawnQuadBatchCommandV1 quadHeader{};
    quadHeader.header.kind = static_cast<uint16_t>(CommandKind::SpawnQuadBatch);
    quadHeader.header.sizeBytes = static_cast<uint16_t>(requiredBytes);
    quadHeader.delayMs = 0u;
    quadHeader.lifeMs = std::clamp<uint32_t>(cmd.ttlMs == 0u ? 640u : cmd.ttlMs, 40u, 15000u);
    quadHeader.itemCount = cmd.itemCount;
    quadHeader.flags = cmd.flags;
    std::memcpy(quadBytes.data(), &quadHeader, sizeof(quadHeader));
    std::memcpy(
        quadBytes.data() + sizeof(quadHeader),
        raw + sizeof(UpsertQuadFieldCommandV1),
        itemBytes);

    if (sizeBytes > requiredBytes) {
        const size_t tailBytes = sizeBytes - requiredBytes;
        quadBytes.resize(sizeof(SpawnQuadBatchCommandV1) + itemBytes + tailBytes, 0u);
        quadHeader.header.sizeBytes = static_cast<uint16_t>(quadBytes.size());
        std::memcpy(quadBytes.data(), &quadHeader, sizeof(quadHeader));
        std::memcpy(
            quadBytes.data() + sizeof(SpawnQuadBatchCommandV1) + itemBytes,
            raw + requiredBytes,
            tailBytes);
    }

    ResolvedSpawnSpriteBatchCommand batch{};
    if (!TryResolveSpawnQuadBatchCommand(
            quadBytes.data(),
            quadBytes.size(),
            config,
            activeManifestPath,
            overlayMotionYUp,
            &batch,
            outError)) {
        return false;
    }

    ResolvedQuadFieldCommand resolved{};
    resolved.fieldId = cmd.fieldId;
    resolved.ttlMs = quadHeader.lifeMs;
    resolved.useGroupLocalOrigin = (cmd.flags & kUpsertQuadFieldFlagUseGroupLocalOrigin) != 0u;
    resolved.batch = std::move(batch);
    resolved.sourceItems = resolved.batch.items;
    const QuadBatchItemV1* sourceItems = reinterpret_cast<const QuadBatchItemV1*>(raw + sizeof(UpsertQuadFieldCommandV1));
    for (size_t index = 0; index < resolved.sourceItems.size(); ++index) {
        resolved.sourceItems[index].localX = sourceItems[index].x;
        resolved.sourceItems[index].localY = sourceItems[index].y;
        resolved.sourceItems[index].rotationRad = sourceItems[index].rotation;
        resolved.sourceItems[index].velocityX = resolved.batch.items[index].velocityX;
        resolved.sourceItems[index].velocityY = resolved.batch.items[index].velocityY;
        resolved.sourceItems[index].accelerationX = resolved.batch.items[index].accelerationX;
        resolved.sourceItems[index].accelerationY = resolved.batch.items[index].accelerationY;
    }
    *outResolved = std::move(resolved);
    return true;
}

inline bool TryResolveRemoveQuadFieldCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    uint32_t* outFieldId,
    std::string* outError) {
    if (!raw || !outFieldId) {
        if (outError) {
            *outError = "remove_quad_field command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(RemoveQuadFieldCommandV1)) {
        if (outError) {
            *outError = "remove_quad_field command truncated";
        }
        return false;
    }

    RemoveQuadFieldCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.fieldId == 0u) {
        if (outError) {
            *outError = "remove_quad_field requires non-zero field_id";
        }
        return false;
    }

    *outFieldId = cmd.fieldId;
    return true;
}

} // namespace mousefx::wasm
