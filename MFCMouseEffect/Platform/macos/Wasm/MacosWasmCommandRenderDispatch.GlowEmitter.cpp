#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmGlowEmitterCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmRetainedGlowEmitterRuntime.h"

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleUpsertGlowEmitterCommand(
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

    mousefx::wasm::ResolvedGlowEmitterCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertGlowEmitterCommand(
            raw,
            sizeBytes,
            config,
            true,
            &resolved,
            &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_glow_emitter command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::UpsertRetainedGlowEmitter(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained glow emitter" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedGlowEmitterCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleRemoveGlowEmitterCommand(
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
    if (!mousefx::wasm::TryResolveRemoveGlowEmitterCommand(raw, sizeBytes, &emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_glow_emitter command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::RemoveRetainedGlowEmitter(activeManifestPath, emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained glow emitter" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedGlowEmitterRemoveCommands += 1;
    outResult->renderedAny = true;
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
