#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
#include "MouseFx/Core/Wasm/WasmPathFillCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"
#include "MouseFx/Core/Wasm/WasmRibbonPathFillResolver.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace mousefx::wasm {

constexpr uint16_t kMaxSpawnRibbonStripPoints = 96u;

inline bool TryResolveSpawnRibbonStripCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    ResolvedSpawnPathFillCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "spawn_ribbon_strip command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(SpawnRibbonStripCommandV1)) {
        if (outError) {
            *outError = "spawn_ribbon_strip command truncated";
        }
        return false;
    }

    SpawnRibbonStripCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.pointCount < 2u) {
        if (outError) {
            *outError = "spawn_ribbon_strip requires at least 2 points";
        }
        return false;
    }
    if (cmd.pointCount > kMaxSpawnRibbonStripPoints) {
        if (outError) {
            *outError = "spawn_ribbon_strip point_count exceeds limit";
        }
        return false;
    }

    const size_t requiredBytes = sizeof(SpawnRibbonStripCommandV1) +
        static_cast<size_t>(cmd.pointCount) * sizeof(RibbonStripPointV1);
    if (requiredBytes > sizeBytes) {
        if (outError) {
            *outError = "spawn_ribbon_strip point payload truncated";
        }
        return false;
    }

    ResolvedSpawnPathFillCommand resolved{};
    resolved.alpha = ClampPathCommandFloat(cmd.alpha, 1.0f, 0.0f, 1.0f);
    resolved.glowWidthPx = ClampPathCommandFloat(cmd.glowWidthPx, 8.0f, 0.0f, 64.0f);
    resolved.delayMs = ResolvePathCommandDelayMs(cmd.delayMs);
    resolved.lifeMs = ResolvePathCommandLifeMs(cmd.lifeMs, static_cast<uint32_t>(config.trail.durationMs));
    resolved.fillRule = kPathFillRuleNonZero;
    if (!TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
            raw,
            sizeBytes,
            requiredBytes,
            false,
            &resolved.semantics,
            outError,
            "spawn_ribbon_strip")) {
        return false;
    }

    const uint32_t themeFillArgb = config.trail.color.value;
    resolved.fillColorArgb = (cmd.fillArgb != 0u) ? cmd.fillArgb : themeFillArgb;
    resolved.glowColorArgb = ResolvePathCommandGlowColor(resolved.fillColorArgb, cmd.glowArgb);
    resolved.fillArgb = ScalePathCommandArgb(resolved.fillColorArgb, resolved.alpha);
    resolved.glowArgb = ScalePathCommandArgb(resolved.glowColorArgb, resolved.alpha);

    std::vector<ResolvedRibbonPointInput> points{};
    points.reserve(cmd.pointCount);
    const size_t pointsOffset = sizeof(SpawnRibbonStripCommandV1);
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
    }

    return TryResolveRibbonPathFillGeometry(
        points,
        (cmd.flags & kSpawnRibbonStripFlagClosed) != 0u,
        resolved.alpha,
        resolved.glowWidthPx,
        resolved.delayMs,
        resolved.lifeMs,
        resolved.fillColorArgb,
        resolved.glowColorArgb,
        resolved.semantics,
        outResolved,
        outError);
}

} // namespace mousefx::wasm
