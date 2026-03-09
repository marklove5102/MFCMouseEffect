#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmPathStrokeCommandConfig.h"
#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

#include <utility>

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleSpawnPathStrokeCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::ResolvedSpawnPathStrokeCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveSpawnPathStrokeCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->droppedCommands += 1;
        outResult->lastError = error.empty() ? "failed to resolve spawn_path_stroke command" : error;
        return true;
    }

    WasmPathStrokeOverlayRequest request{};
    request.frameLeftPx = resolved.frameLeftPx;
    request.frameTopPx = resolved.frameTopPx;
    request.squareSizePx = resolved.squareSizePx;
    request.nodes.reserve(resolved.localNodes.size());
    for (const mousefx::wasm::ResolvedPathStrokeNode& node : resolved.localNodes) {
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
    request.lineWidthPx = resolved.lineWidthPx;
    request.alpha = resolved.alpha;
    request.strokeArgb = resolved.strokeColorArgb;
    request.glowArgb = resolved.glowColorArgb;
    request.delayMs = resolved.delayMs;
    request.lifeMs = resolved.lifeMs;
    request.lineJoin = resolved.lineJoin;
    request.lineCap = resolved.lineCap;
    request.semantics = resolved.semantics;

    const WasmOverlayRenderResult renderResult = ShowWasmPathStrokeOverlay(request);
    if (renderResult == WasmOverlayRenderResult::Rendered) {
        outResult->executedPathStrokeCommands += 1;
        outResult->renderedAny = true;
    } else if (!AccountThrottle(renderResult, false, outResult, outThrottleCounters)) {
        outResult->droppedCommands += 1;
        outResult->lastError = "failed to render spawn_path_stroke command";
    }
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
