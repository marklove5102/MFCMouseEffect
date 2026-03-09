#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmGlowBatchCommandConfig.h"
#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

#include <utility>

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleSpawnGlowBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::ResolvedSpawnGlowBatchCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveSpawnGlowBatchCommand(raw, sizeBytes, config, true, &resolved, &error)) {
        outResult->droppedCommands += 1;
        outResult->lastError = error.empty() ? "failed to resolve spawn_glow_batch command" : error;
        return true;
    }

    WasmGlowBatchOverlayRequest request{};
    request.frameLeftPx = resolved.frameLeftPx;
    request.frameTopPx = resolved.frameTopPx;
    request.squareSizePx = resolved.squareSizePx;
    request.delayMs = resolved.delayMs;
    request.lifeMs = resolved.lifeMs;
    request.semantics = resolved.semantics;
    request.particles.reserve(resolved.items.size());
    for (const mousefx::wasm::ResolvedGlowBatchItem& item : resolved.items) {
        request.particles.push_back(WasmGlowBatchOverlayParticle{
            item.localX,
            item.localY,
            item.radiusPx,
            item.alpha,
            item.colorArgb,
            item.velocityX,
            item.velocityY,
            item.accelerationX,
            item.accelerationY,
        });
    }

    const WasmOverlayRenderResult renderResult = ShowWasmGlowBatchOverlay(request);
    if (renderResult == WasmOverlayRenderResult::Rendered) {
        outResult->executedGlowBatchCommands += 1;
        outResult->renderedAny = true;
    } else if (!AccountThrottle(renderResult, false, outResult, outThrottleCounters)) {
        outResult->droppedCommands += 1;
        outResult->lastError = "failed to render spawn_glow_batch command";
    }
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
