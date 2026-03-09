#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Wasm/WasmPathCommandGeometry.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
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

using ResolvedPathStrokeNode = ResolvedPathNode;

struct ResolvedSpawnPathStrokeCommand final {
    ScreenPoint centerScreenPt{};
    int frameLeftPx = 0;
    int frameTopPx = 0;
    int squareSizePx = 64;
    std::vector<ResolvedPathStrokeNode> localNodes{};
    float lineWidthPx = 4.0f;
    float alpha = 1.0f;
    uint32_t delayMs = 0;
    uint32_t lifeMs = 320;
    uint32_t strokeColorArgb = 0xFFFFFFFFu;
    uint32_t glowColorArgb = 0x66FFFFFFu;
    uint32_t strokeArgb = 0xFFFFFFFFu;
    uint32_t glowArgb = 0x66FFFFFFu;
    uint8_t lineJoin = kPathStrokeLineJoinRound;
    uint8_t lineCap = kPathStrokeLineCapRound;
    RenderSemantics semantics{};
};

inline float ClampSpawnPathStrokeFloat(float value, float fallback, float minValue, float maxValue) {
    return ClampPathCommandFloat(value, fallback, minValue, maxValue);
}

inline uint32_t ResolveSpawnPathStrokeLifeMs(uint32_t lifeMs, uint32_t fallbackLifeMs) {
    return ResolvePathCommandLifeMs(lifeMs, fallbackLifeMs);
}

inline uint32_t ResolveSpawnPathStrokeDelayMs(uint32_t delayMs) {
    return ResolvePathCommandDelayMs(delayMs);
}

inline uint32_t ScaleSpawnPathStrokeArgb(uint32_t argb, float alphaScale) {
    return ScalePathCommandArgb(argb, alphaScale);
}

inline uint32_t ResolveSpawnPathStrokeGlowColor(uint32_t strokeArgb, uint32_t glowArgb) {
    return ResolvePathCommandGlowColor(strokeArgb, glowArgb);
}

inline bool IsSupportedPathStrokeLineJoin(uint8_t lineJoin) {
    return lineJoin == kPathStrokeLineJoinMiter ||
        lineJoin == kPathStrokeLineJoinRound ||
        lineJoin == kPathStrokeLineJoinBevel;
}

inline bool IsSupportedPathStrokeLineCap(uint8_t lineCap) {
    return lineCap == kPathStrokeLineCapButt ||
        lineCap == kPathStrokeLineCapRound ||
        lineCap == kPathStrokeLineCapSquare;
}

inline bool TryResolveSpawnPathStrokeCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    ResolvedSpawnPathStrokeCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "spawn_path_stroke command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(SpawnPathStrokeCommandV1)) {
        if (outError) {
            *outError = "spawn_path_stroke command truncated";
        }
        return false;
    }

    SpawnPathStrokeCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.nodeCount == 0u) {
        if (outError) {
            *outError = "spawn_path_stroke requires at least 1 node";
        }
        return false;
    }
    if (cmd.nodeCount > kMaxSpawnPathNodes) {
        if (outError) {
            *outError = "spawn_path_stroke node_count exceeds limit";
        }
        return false;
    }
    if (!IsSupportedPathStrokeLineJoin(cmd.lineJoin)) {
        if (outError) {
            *outError = "spawn_path_stroke line_join is unsupported";
        }
        return false;
    }
    if (!IsSupportedPathStrokeLineCap(cmd.lineCap)) {
        if (outError) {
            *outError = "spawn_path_stroke line_cap is unsupported";
        }
        return false;
    }

    const size_t requiredBytes = sizeof(SpawnPathStrokeCommandV1) +
        static_cast<size_t>(cmd.nodeCount) * sizeof(PathStrokeNodeV1);
    if (requiredBytes > sizeBytes) {
        if (outError) {
            *outError = "spawn_path_stroke node payload truncated";
        }
        return false;
    }

    ResolvedSpawnPathStrokeCommand resolved{};
    resolved.lineWidthPx = ClampSpawnPathStrokeFloat(cmd.lineWidthPx, config.trail.lineWidth, 0.5f, 48.0f);
    resolved.alpha = ClampSpawnPathStrokeFloat(cmd.alpha, 1.0f, 0.0f, 1.0f);
    resolved.delayMs = ResolveSpawnPathStrokeDelayMs(cmd.delayMs);
    resolved.lifeMs = ResolveSpawnPathStrokeLifeMs(cmd.lifeMs, static_cast<uint32_t>(config.trail.durationMs));
    resolved.lineJoin = cmd.lineJoin;
    resolved.lineCap = cmd.lineCap;
    if (!TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
            raw,
            sizeBytes,
            requiredBytes,
            false,
            &resolved.semantics,
            outError,
            "spawn_path_stroke")) {
        return false;
    }

    const uint32_t themeStrokeArgb = config.trail.color.value;
    resolved.strokeColorArgb = (cmd.strokeArgb != 0u) ? cmd.strokeArgb : themeStrokeArgb;
    resolved.glowColorArgb = ResolveSpawnPathStrokeGlowColor(resolved.strokeColorArgb, cmd.glowArgb);
    resolved.strokeArgb = ScaleSpawnPathStrokeArgb(resolved.strokeColorArgb, resolved.alpha);
    resolved.glowArgb = ScaleSpawnPathStrokeArgb(resolved.glowColorArgb, resolved.alpha);
    const int paddingPx = std::max(12, static_cast<int>(std::ceil(resolved.lineWidthPx * 4.0f)));
    ResolvedPathGeometry geometry{};
    if (!TryResolvePathCommandGeometry(
            raw,
            sizeBytes,
            sizeof(SpawnPathStrokeCommandV1),
            cmd.nodeCount,
            paddingPx,
            "spawn_path_stroke",
            &geometry,
            outError)) {
        return false;
    }
    resolved.centerScreenPt = geometry.centerScreenPt;
    resolved.frameLeftPx = geometry.frameLeftPx;
    resolved.frameTopPx = geometry.frameTopPx;
    resolved.squareSizePx = geometry.squareSizePx;
    resolved.localNodes = std::move(geometry.localNodes);
    *outResolved = std::move(resolved);
    return true;
}

} // namespace mousefx::wasm
