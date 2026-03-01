#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderResolvers.h"
#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace mousefx::platform::macos::wasm_render_dispatch {

namespace {

TextConfig BuildSpawnTextConfig(const TextConfig& baseConfig, const mousefx::wasm::SpawnTextCommandV1& cmd) {
    TextConfig cfg = baseConfig;
    if (cmd.lifeMs > 0) {
        cfg.durationMs = std::clamp<int>(static_cast<int>(cmd.lifeMs), 80, 8000);
    }

    const float lifeSeconds = std::max(0.08f, static_cast<float>(cfg.durationMs) / 1000.0f);
    const float predictedDy = (cmd.vy * lifeSeconds) + (0.5f * cmd.ay * lifeSeconds * lifeSeconds);
    const float fallbackDy = std::abs(cmd.vy) * 0.55f;
    const float distance = std::max(std::abs(predictedDy), fallbackDy);
    cfg.floatDistance = std::clamp<int>(static_cast<int>(std::lround(distance)), 16, 420);

    if (cmd.scale > 0.0f) {
        const float scaledSize = cfg.fontSize * cmd.scale;
        cfg.fontSize = std::clamp(scaledSize, 6.0f, 90.0f);
    }
    return cfg;
}

} // namespace

bool HandleSpawnTextCommand(
    const uint8_t* raw,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::SpawnTextCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    const ScreenPoint pt{
        static_cast<int32_t>(std::lround(cmd.x)),
        static_cast<int32_t>(std::lround(cmd.y)),
    };
    const std::wstring text = wasm_render_resolver::ResolveTextById(config, cmd.textId);
    const uint32_t color = wasm_render_resolver::ResolveTextColorArgb(config, cmd.textId, cmd.colorRgba);
    const TextConfig textConfig = BuildSpawnTextConfig(config.textClick, cmd);
    const WasmOverlayRenderResult renderResult =
        ShowWasmTextOverlay(pt, text, color, textConfig);
    if (renderResult == WasmOverlayRenderResult::Rendered) {
        outResult->executedTextCommands += 1;
        outResult->renderedAny = true;
    } else if (!AccountThrottle(renderResult, true, outResult, outThrottleCounters)) {
        outResult->droppedCommands += 1;
        outResult->lastError = "failed to render spawn_text command";
    }
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
