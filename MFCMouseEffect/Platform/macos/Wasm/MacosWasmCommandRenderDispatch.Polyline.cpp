#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmPolylineCommandConfig.h"
#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

#include <utility>

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleSpawnPolylineCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::ResolvedSpawnPolylineCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveSpawnPolylineCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->droppedCommands += 1;
        outResult->lastError = error.empty() ? "failed to resolve spawn_polyline command" : error;
        return true;
    }

    WasmPolylineOverlayRequest request{};
    request.frameLeftPx = resolved.frameLeftPx;
    request.frameTopPx = resolved.frameTopPx;
    request.squareSizePx = resolved.squareSizePx;
    request.localPointsXY = std::move(resolved.localPointsXY);
    request.lineWidthPx = resolved.lineWidthPx;
    request.alpha = resolved.alpha;
    request.strokeArgb = resolved.strokeColorArgb;
    request.glowArgb = resolved.glowColorArgb;
    request.delayMs = resolved.delayMs;
    request.lifeMs = resolved.lifeMs;
    request.closed = resolved.closed;

    const WasmOverlayRenderResult renderResult = ShowWasmPolylineOverlay(request);
    if (renderResult == WasmOverlayRenderResult::Rendered) {
        outResult->executedPolylineCommands += 1;
        outResult->renderedAny = true;
    } else if (!AccountThrottle(renderResult, false, outResult, outThrottleCounters)) {
        outResult->droppedCommands += 1;
        outResult->lastError = "failed to render spawn_polyline command";
    }
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
