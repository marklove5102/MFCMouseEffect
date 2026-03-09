#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
#include "MouseFx/Core/Wasm/WasmPathCommandGeometry.h"
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

using ResolvedPathFillNode = ResolvedPathNode;

struct ResolvedSpawnPathFillCommand final {
    ScreenPoint centerScreenPt{};
    int frameLeftPx = 0;
    int frameTopPx = 0;
    int squareSizePx = 64;
    std::vector<ResolvedPathFillNode> localNodes{};
    float alpha = 1.0f;
    float glowWidthPx = 10.0f;
    uint32_t delayMs = 0;
    uint32_t lifeMs = 320;
    uint32_t fillColorArgb = 0xFFFFFFFFu;
    uint32_t glowColorArgb = 0x66FFFFFFu;
    uint32_t fillArgb = 0xFFFFFFFFu;
    uint32_t glowArgb = 0x66FFFFFFu;
    uint8_t fillRule = kPathFillRuleNonZero;
    RenderSemantics semantics{};
};

inline bool IsSupportedPathFillRule(uint8_t fillRule) {
    return fillRule == kPathFillRuleNonZero || fillRule == kPathFillRuleEvenOdd;
}

inline bool TryResolveSpawnPathFillCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    ResolvedSpawnPathFillCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "spawn_path_fill command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(SpawnPathFillCommandV1)) {
        if (outError) {
            *outError = "spawn_path_fill command truncated";
        }
        return false;
    }

    SpawnPathFillCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.nodeCount == 0u) {
        if (outError) {
            *outError = "spawn_path_fill requires at least 1 node";
        }
        return false;
    }
    if (cmd.nodeCount > kMaxSpawnPathNodes) {
        if (outError) {
            *outError = "spawn_path_fill node_count exceeds limit";
        }
        return false;
    }
    if (!IsSupportedPathFillRule(cmd.fillRule)) {
        if (outError) {
            *outError = "spawn_path_fill fill_rule is unsupported";
        }
        return false;
    }

    const size_t requiredBytes = sizeof(SpawnPathFillCommandV1) +
        static_cast<size_t>(cmd.nodeCount) * sizeof(PathStrokeNodeV1);
    if (requiredBytes > sizeBytes) {
        if (outError) {
            *outError = "spawn_path_fill node payload truncated";
        }
        return false;
    }

    ResolvedSpawnPathFillCommand resolved{};
    resolved.alpha = ClampPathCommandFloat(cmd.alpha, 1.0f, 0.0f, 1.0f);
    resolved.glowWidthPx = ClampPathCommandFloat(cmd.glowWidthPx, 10.0f, 0.0f, 64.0f);
    resolved.delayMs = ResolvePathCommandDelayMs(cmd.delayMs);
    resolved.lifeMs = ResolvePathCommandLifeMs(cmd.lifeMs, static_cast<uint32_t>(config.trail.durationMs));
    resolved.fillRule = cmd.fillRule;
    if (!TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
            raw,
            sizeBytes,
            requiredBytes,
            false,
            &resolved.semantics,
            outError,
            "spawn_path_fill")) {
        return false;
    }

    const uint32_t themeFillArgb = config.trail.color.value;
    resolved.fillColorArgb = (cmd.fillArgb != 0u) ? cmd.fillArgb : themeFillArgb;
    resolved.glowColorArgb = ResolvePathCommandGlowColor(resolved.fillColorArgb, cmd.glowArgb);
    resolved.fillArgb = ScalePathCommandArgb(resolved.fillColorArgb, resolved.alpha);
    resolved.glowArgb = ScalePathCommandArgb(resolved.glowColorArgb, resolved.alpha);

    const int paddingPx = std::max(14, static_cast<int>(std::ceil(resolved.glowWidthPx * 2.5f)));
    ResolvedPathGeometry geometry{};
    if (!TryResolvePathCommandGeometry(
            raw,
            sizeBytes,
            sizeof(SpawnPathFillCommandV1),
            cmd.nodeCount,
            paddingPx,
            "spawn_path_fill",
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
