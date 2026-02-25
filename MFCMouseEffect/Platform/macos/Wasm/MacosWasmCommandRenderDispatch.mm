#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderResolvers.h"
#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <cmath>
#include <cstring>

namespace mousefx::platform::macos::wasm_render_dispatch {

namespace {

bool AccountThrottle(
    WasmOverlayRenderResult renderResult,
    bool isText,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    if (!outResult || !outThrottleCounters) {
        return false;
    }

    if (renderResult == WasmOverlayRenderResult::ThrottledByCapacity) {
        outThrottleCounters->byCapacity += 1;
    } else if (renderResult == WasmOverlayRenderResult::ThrottledByInterval) {
        outThrottleCounters->byInterval += 1;
    } else {
        return false;
    }

    if (isText) {
        outThrottleCounters->text += 1;
    } else {
        outThrottleCounters->image += 1;
    }
    outResult->droppedCommands += 1;
    return true;
}

} // namespace

bool ExecuteParsedCommand(
    const mousefx::wasm::CommandRecord& record,
    const uint8_t* commandBuffer,
    size_t commandBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    if (!commandBuffer || !outResult || !outThrottleCounters) {
        return false;
    }

    if (record.offsetBytes + record.sizeBytes > commandBytes) {
        outResult->droppedCommands += 1;
        return true;
    }

    const uint8_t* raw = commandBuffer + record.offsetBytes;
    switch (record.kind) {
    case mousefx::wasm::CommandKind::SpawnText: {
        mousefx::wasm::SpawnTextCommandV1 cmd{};
        std::memcpy(&cmd, raw, sizeof(cmd));
        const ScreenPoint pt{
            static_cast<int32_t>(std::lround(cmd.x)),
            static_cast<int32_t>(std::lround(cmd.y)),
        };
        const std::wstring text = wasm_render_resolver::ResolveTextById(config, cmd.textId);
        const uint32_t color = wasm_render_resolver::ResolveTextColorArgb(config, cmd.textId, cmd.colorRgba);
        const WasmOverlayRenderResult renderResult =
            ShowWasmTextOverlay(pt, text, color, cmd.scale, cmd.lifeMs);
        if (renderResult == WasmOverlayRenderResult::Rendered) {
            outResult->executedTextCommands += 1;
            outResult->renderedAny = true;
        } else if (!AccountThrottle(renderResult, true, outResult, outThrottleCounters)) {
            outResult->droppedCommands += 1;
            outResult->lastError = "failed to render spawn_text command";
        }
        return true;
    }
    case mousefx::wasm::CommandKind::SpawnImage: {
        mousefx::wasm::SpawnImageCommandV1 cmd{};
        std::memcpy(&cmd, raw, sizeof(cmd));
        WasmImageOverlayRequest request{};
        request.screenPt.x = static_cast<int32_t>(std::lround(cmd.x));
        request.screenPt.y = static_cast<int32_t>(std::lround(cmd.y));
        request.assetPath = wasm_render_resolver::ResolveImageAssetPath(activeManifestPath, cmd.imageId);
        request.tintArgb = wasm_render_resolver::ResolveImageTintArgb(config, cmd.tintRgba);
        request.scale = cmd.scale;
        request.alpha = cmd.alpha;
        request.lifeMs = cmd.lifeMs;
        request.delayMs = cmd.delayMs;
        request.velocityX = cmd.vx;
        request.velocityY = cmd.vy;
        request.accelerationX = cmd.ax;
        request.accelerationY = cmd.ay;
        request.rotationRad = cmd.rotation;
        request.applyTint = wasm_render_resolver::HasVisibleAlpha(cmd.tintRgba);
        const WasmOverlayRenderResult renderResult = ShowWasmImageOverlay(request);
        if (renderResult == WasmOverlayRenderResult::Rendered) {
            outResult->executedImageCommands += 1;
            outResult->renderedAny = true;
        } else if (!AccountThrottle(renderResult, false, outResult, outThrottleCounters)) {
            outResult->droppedCommands += 1;
            outResult->lastError = "failed to render spawn_image command";
        }
        return true;
    }
    case mousefx::wasm::CommandKind::SpawnImageAffine: {
        mousefx::wasm::SpawnImageAffineCommandV1 cmd{};
        std::memcpy(&cmd, raw, sizeof(cmd));
        WasmImageOverlayRequest request{};
        request.screenPt.x = static_cast<int32_t>(std::lround(cmd.base.x + cmd.affineDx));
        request.screenPt.y = static_cast<int32_t>(std::lround(cmd.base.y + cmd.affineDy));
        request.assetPath = wasm_render_resolver::ResolveImageAssetPath(activeManifestPath, cmd.base.imageId);
        request.tintArgb = wasm_render_resolver::ResolveImageTintArgb(config, cmd.base.tintRgba);
        request.scale = cmd.base.scale;
        request.alpha = cmd.base.alpha;
        request.lifeMs = cmd.base.lifeMs;
        request.delayMs = cmd.base.delayMs;
        request.velocityX = cmd.base.vx;
        request.velocityY = cmd.base.vy;
        request.accelerationX = cmd.base.ax;
        request.accelerationY = cmd.base.ay;
        request.rotationRad = cmd.base.rotation;
        request.applyTint = wasm_render_resolver::HasVisibleAlpha(cmd.base.tintRgba);
        const WasmOverlayRenderResult renderResult = ShowWasmImageOverlay(request);
        if (renderResult == WasmOverlayRenderResult::Rendered) {
            outResult->executedImageCommands += 1;
            outResult->renderedAny = true;
        } else if (!AccountThrottle(renderResult, false, outResult, outThrottleCounters)) {
            outResult->droppedCommands += 1;
            outResult->lastError = "failed to render spawn_image_affine command";
        }
        return true;
    }
    default:
        outResult->droppedCommands += 1;
        return true;
    }
}

void ApplyThrottleCounters(const ThrottleCounters& counters, mousefx::wasm::CommandExecutionResult* outResult) {
    if (!outResult) {
        return;
    }
    outResult->throttledCommands = counters.text + counters.image;
    outResult->throttledByCapacityCommands = counters.byCapacity;
    outResult->throttledByIntervalCommands = counters.byInterval;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
