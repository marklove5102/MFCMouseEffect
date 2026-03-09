#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmQuadBatchCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmSpriteBatchCommandConfig.h"
#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

namespace mousefx::platform::macos::wasm_render_dispatch {

namespace {

bool RenderResolvedSpriteBatch(
    mousefx::wasm::ResolvedSpawnSpriteBatchCommand resolved,
    const char* failureMessage,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    WasmSpriteBatchOverlayRequest request{};
    request.frameLeftPx = resolved.frameLeftPx;
    request.frameTopPx = resolved.frameTopPx;
    request.squareSizePx = resolved.squareSizePx;
    request.delayMs = resolved.delayMs;
    request.lifeMs = resolved.lifeMs;
    request.semantics = resolved.semantics;
    request.sprites.reserve(resolved.items.size());
    for (const mousefx::wasm::ResolvedSpriteBatchItem& item : resolved.items) {
        request.sprites.push_back(WasmSpriteBatchOverlaySprite{
            item.assetPath,
            item.localX,
            item.localY,
            item.widthPx,
            item.heightPx,
            item.alpha,
            item.rotationRad,
            item.tintArgb,
            item.applyTint,
            item.srcU0,
            item.srcV0,
            item.srcU1,
            item.srcV1,
            item.velocityX,
            item.velocityY,
            item.accelerationX,
            item.accelerationY,
        });
    }

    const WasmOverlayRenderResult renderResult = ShowWasmSpriteBatchOverlay(request);
    if (renderResult == WasmOverlayRenderResult::Rendered) {
        outResult->executedSpriteBatchCommands += 1;
        outResult->renderedAny = true;
    } else if (!AccountThrottle(renderResult, false, outResult, outThrottleCounters)) {
        outResult->droppedCommands += 1;
        outResult->lastError = failureMessage ? failureMessage : "failed to render sprite batch command";
    }
    return true;
}

} // namespace

bool HandleSpawnSpriteBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::ResolvedSpawnSpriteBatchCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveSpawnSpriteBatchCommand(
            raw,
            sizeBytes,
            config,
            activeManifestPath,
            true,
            &resolved,
            &error)) {
        outResult->droppedCommands += 1;
        outResult->lastError = error.empty() ? "failed to resolve spawn_sprite_batch command" : error;
        return true;
    }

    return RenderResolvedSpriteBatch(
        std::move(resolved),
        "failed to render spawn_sprite_batch command",
        outResult,
        outThrottleCounters);
}

bool HandleSpawnQuadBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    mousefx::wasm::ResolvedSpawnSpriteBatchCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveSpawnQuadBatchCommand(
            raw,
            sizeBytes,
            config,
            activeManifestPath,
            true,
            &resolved,
            &error)) {
        outResult->droppedCommands += 1;
        outResult->lastError = error.empty() ? "failed to resolve spawn_quad_batch command" : error;
        return true;
    }

    return RenderResolvedSpriteBatch(
        std::move(resolved),
        "failed to render spawn_quad_batch command",
        outResult,
        outThrottleCounters);
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
