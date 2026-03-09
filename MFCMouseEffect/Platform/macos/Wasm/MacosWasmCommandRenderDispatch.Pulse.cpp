#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"
#include "MouseFx/Core/Wasm/WasmPulseCommandConfig.h"

#include <cstring>

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleSpawnPulseCommand(
    const uint8_t* raw,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::SpawnPulseCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    const mousefx::wasm::ResolvedSpawnPulseCommand resolved = mousefx::wasm::ResolveSpawnPulseCommand(config, cmd);

    WasmPulseOverlayRequest request{};
    request.screenPt = resolved.screenPt;
    request.normalizedType = resolved.normalizedType;
    request.sizePx = resolved.style.windowSize;
    request.alpha = resolved.alpha;
    request.startRadiusPx = resolved.style.startRadius;
    request.endRadiusPx = resolved.style.endRadius;
    request.strokeWidthPx = resolved.style.strokeWidth;
    request.fillArgb = resolved.fillColorArgb;
    request.strokeArgb = resolved.strokeColorArgb;
    request.glowArgb = resolved.glowColorArgb;
    request.delayMs = resolved.delayMs;
    request.lifeMs = resolved.lifeMs;

    const WasmOverlayRenderResult renderResult = ShowWasmPulseOverlay(request);
    if (renderResult == WasmOverlayRenderResult::Rendered) {
        outResult->executedPulseCommands += 1;
        outResult->renderedAny = true;
    } else if (!AccountThrottle(renderResult, false, outResult, outThrottleCounters)) {
        outResult->droppedCommands += 1;
        outResult->lastError = "failed to render spawn_pulse command";
    }
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
