#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmQuadFieldCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmRetainedQuadFieldRuntime.h"

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleUpsertQuadFieldCommand(
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

    mousefx::wasm::ResolvedQuadFieldCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertQuadFieldCommand(
            raw,
            sizeBytes,
            config,
            activeManifestPath,
            true,
            &resolved,
            &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_quad_field command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::UpsertRetainedQuadField(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained quad field" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedQuadFieldCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleRemoveQuadFieldCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    uint32_t fieldId = 0u;
    std::string error;
    if (!mousefx::wasm::TryResolveRemoveQuadFieldCommand(raw, sizeBytes, &fieldId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_quad_field command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::RemoveRetainedQuadField(activeManifestPath, fieldId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained quad field" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedQuadFieldRemoveCommands += 1;
    outResult->renderedAny = true;
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
