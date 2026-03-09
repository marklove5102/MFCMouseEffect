#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmRetainedSpriteEmitterRuntime.h"
#include "MouseFx/Core/Wasm/WasmSpriteEmitterCommandConfig.h"

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleUpsertSpriteEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    mousefx::wasm::ResolvedSpriteEmitterCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertSpriteEmitterCommand(
            raw,
            sizeBytes,
            config,
            activeManifestPath,
            true,
            &resolved,
            &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_sprite_emitter command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::UpsertRetainedSpriteEmitter(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained sprite emitter" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedSpriteEmitterCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleRemoveSpriteEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    uint32_t emitterId = 0u;
    std::string error;
    if (!mousefx::wasm::TryResolveRemoveSpriteEmitterCommand(raw, sizeBytes, &emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_sprite_emitter command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::RemoveRetainedSpriteEmitter(activeManifestPath, emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained sprite emitter" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedSpriteEmitterRemoveCommands += 1;
    outResult->renderedAny = true;
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
