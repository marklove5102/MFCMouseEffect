#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
#include "MouseFx/Core/Wasm/WasmRibbonPathFillResolver.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace mousefx::wasm {

constexpr uint16_t kMaxRetainedRibbonTrailPoints = 96u;

struct ResolvedRibbonTrailSourcePoint final {
    float x = 0.0f;
    float y = 0.0f;
    float widthPx = 12.0f;
};

struct ResolvedRibbonTrailCommand final {
    uint32_t trailId = 0u;
    uint32_t ttlMs = 640u;
    bool useGroupLocalOrigin = false;
    bool closed = false;
    std::vector<ResolvedRibbonTrailSourcePoint> sourcePoints{};
    ResolvedSpawnPathFillCommand pathFill{};
};

inline bool TryResolveUpsertRibbonTrailCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    ResolvedRibbonTrailCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_ribbon_trail command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertRibbonTrailCommandV1)) {
        if (outError) {
            *outError = "upsert_ribbon_trail command truncated";
        }
        return false;
    }

    UpsertRibbonTrailCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.trailId == 0u) {
        if (outError) {
            *outError = "upsert_ribbon_trail trail_id must be non-zero";
        }
        return false;
    }
    if (cmd.pointCount < 2u) {
        if (outError) {
            *outError = "upsert_ribbon_trail requires at least 2 points";
        }
        return false;
    }
    if (cmd.pointCount > kMaxRetainedRibbonTrailPoints) {
        if (outError) {
            *outError = "upsert_ribbon_trail point_count exceeds limit";
        }
        return false;
    }

    const size_t payloadBytes =
        sizeof(UpsertRibbonTrailCommandV1) + static_cast<size_t>(cmd.pointCount) * sizeof(RibbonStripPointV1);
    if (payloadBytes > sizeBytes) {
        if (outError) {
            *outError = "upsert_ribbon_trail point payload truncated";
        }
        return false;
    }

    ResolvedRibbonTrailCommand resolved{};
    resolved.trailId = cmd.trailId;
    resolved.ttlMs = std::clamp<uint32_t>(cmd.ttlMs, 40u, 15000u);
    resolved.useGroupLocalOrigin = (cmd.flags & kUpsertRibbonTrailFlagUseGroupLocalOrigin) != 0u;
    resolved.closed = (cmd.flags & kUpsertRibbonTrailFlagClosed) != 0u;
    resolved.pathFill.alpha = ClampPathCommandFloat(cmd.alpha, 1.0f, 0.0f, 1.0f);
    resolved.pathFill.glowWidthPx = ClampPathCommandFloat(cmd.glowWidthPx, 8.0f, 0.0f, 64.0f);
    resolved.pathFill.delayMs = 0u;
    resolved.pathFill.lifeMs = resolved.ttlMs;
    resolved.pathFill.fillRule = kPathFillRuleNonZero;
    const uint32_t themeFillArgb = config.trail.color.value;
    resolved.pathFill.fillColorArgb = (cmd.fillArgb != 0u) ? cmd.fillArgb : themeFillArgb;
    resolved.pathFill.glowColorArgb = ResolvePathCommandGlowColor(resolved.pathFill.fillColorArgb, cmd.glowArgb);
    if (!TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
            raw,
            sizeBytes,
            payloadBytes,
            false,
            &resolved.pathFill.semantics,
            outError,
            "upsert_ribbon_trail")) {
        return false;
    }

    std::vector<ResolvedRibbonPointInput> points{};
    points.reserve(cmd.pointCount);
    resolved.sourcePoints.reserve(cmd.pointCount);
    const size_t pointsOffset = sizeof(UpsertRibbonTrailCommandV1);
    for (uint16_t index = 0; index < cmd.pointCount; ++index) {
        RibbonStripPointV1 point{};
        std::memcpy(
            &point,
            raw + pointsOffset + static_cast<size_t>(index) * sizeof(RibbonStripPointV1),
            sizeof(point));
        points.push_back(ResolvedRibbonPointInput{
            ClampPathPoint(point.x, point.y),
            ClampPathCommandFloat(point.widthPx, 12.0f, 1.0f, 240.0f),
        });
        resolved.sourcePoints.push_back(ResolvedRibbonTrailSourcePoint{
            point.x,
            point.y,
            ClampPathCommandFloat(point.widthPx, 12.0f, 1.0f, 240.0f),
        });
    }

    if (!TryResolveRibbonPathFillGeometry(
            points,
            resolved.closed,
            resolved.pathFill.alpha,
            resolved.pathFill.glowWidthPx,
            0u,
            resolved.ttlMs,
            resolved.pathFill.fillColorArgb,
            resolved.pathFill.glowColorArgb,
            resolved.pathFill.semantics,
            &resolved.pathFill,
            outError)) {
        return false;
    }

    *outResolved = std::move(resolved);
    return true;
}

inline bool TryResolveRemoveRibbonTrailCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    uint32_t* outTrailId,
    std::string* outError) {
    if (!raw || !outTrailId) {
        if (outError) {
            *outError = "remove_ribbon_trail command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(RemoveRibbonTrailCommandV1)) {
        if (outError) {
            *outError = "remove_ribbon_trail command truncated";
        }
        return false;
    }

    RemoveRibbonTrailCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.trailId == 0u) {
        if (outError) {
            *outError = "remove_ribbon_trail trail_id must be non-zero";
        }
        return false;
    }

    *outTrailId = cmd.trailId;
    return true;
}

} // namespace mousefx::wasm
