#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmPathFillCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmRibbonStripCommandConfig.h"
#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

#include <utility>

namespace mousefx::platform::macos::wasm_render_dispatch {

namespace {

bool RenderResolvedPathFill(
    mousefx::wasm::ResolvedSpawnPathFillCommand resolved,
    const char* failureMessage,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    WasmPathFillOverlayRequest request{};
    request.frameLeftPx = resolved.frameLeftPx;
    request.frameTopPx = resolved.frameTopPx;
    request.squareSizePx = resolved.squareSizePx;
    request.nodes.reserve(resolved.localNodes.size());
    for (const mousefx::wasm::ResolvedPathFillNode& node : resolved.localNodes) {
        request.nodes.push_back(WasmPathStrokeOverlayNode{
            node.opcode,
            node.x1,
            node.y1,
            node.x2,
            node.y2,
            node.x3,
            node.y3,
        });
    }
    request.alpha = resolved.alpha;
    request.glowWidthPx = resolved.glowWidthPx;
    request.fillArgb = resolved.fillColorArgb;
    request.glowArgb = resolved.glowColorArgb;
    request.delayMs = resolved.delayMs;
    request.lifeMs = resolved.lifeMs;
    request.fillRule = resolved.fillRule;
    request.semantics = resolved.semantics;

    const WasmOverlayRenderResult renderResult = ShowWasmPathFillOverlay(request);
    if (renderResult == WasmOverlayRenderResult::Rendered) {
        outResult->executedPathFillCommands += 1;
        outResult->renderedAny = true;
    } else if (!AccountThrottle(renderResult, false, outResult, outThrottleCounters)) {
        outResult->droppedCommands += 1;
        outResult->lastError = failureMessage ? failureMessage : "failed to render spawn_path_fill command";
    }
    return true;
}

} // namespace

bool HandleSpawnPathFillCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::ResolvedSpawnPathFillCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveSpawnPathFillCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->droppedCommands += 1;
        outResult->lastError = error.empty() ? "failed to resolve spawn_path_fill command" : error;
        return true;
    }

    return RenderResolvedPathFill(
        std::move(resolved),
        "failed to render spawn_path_fill command",
        outResult,
        outThrottleCounters);
}

bool HandleSpawnRibbonStripCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::ResolvedSpawnPathFillCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveSpawnRibbonStripCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->droppedCommands += 1;
        outResult->lastError = error.empty() ? "failed to resolve spawn_ribbon_strip command" : error;
        return true;
    }

    return RenderResolvedPathFill(
        std::move(resolved),
        "failed to render spawn_ribbon_strip command",
        outResult,
        outThrottleCounters);
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
