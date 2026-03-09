#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmParticleEmitterCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmRetainedParticleEmitterRuntime.h"

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleUpsertParticleEmitterCommand(
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

    mousefx::wasm::ResolvedParticleEmitterCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertParticleEmitterCommand(
            raw,
            sizeBytes,
            config,
            true,
            &resolved,
            &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_particle_emitter command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::UpsertRetainedParticleEmitter(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained particle emitter" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedParticleEmitterCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleRemoveParticleEmitterCommand(
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
    if (!mousefx::wasm::TryResolveRemoveParticleEmitterCommand(raw, sizeBytes, &emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_particle_emitter command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::RemoveRetainedParticleEmitter(activeManifestPath, emitterId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained particle emitter" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedParticleEmitterRemoveCommands += 1;
    outResult->renderedAny = true;
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
