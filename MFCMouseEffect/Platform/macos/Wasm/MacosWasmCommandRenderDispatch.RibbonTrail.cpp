#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmRetainedRibbonTrailRuntime.h"
#include "MouseFx/Core/Wasm/WasmRibbonTrailCommandConfig.h"

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleUpsertRibbonTrailCommand(
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

    mousefx::wasm::ResolvedRibbonTrailCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertRibbonTrailCommand(raw, sizeBytes, config, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_ribbon_trail command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::UpsertRetainedRibbonTrail(activeManifestPath, resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to upsert retained ribbon trail" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedRibbonTrailCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleRemoveRibbonTrailCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    uint32_t trailId = 0u;
    std::string error;
    if (!mousefx::wasm::TryResolveRemoveRibbonTrailCommand(raw, sizeBytes, &trailId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_ribbon_trail command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (!mousefx::wasm::RemoveRetainedRibbonTrail(activeManifestPath, trailId, &error)) {
        outResult->lastError = error.empty() ? "failed to remove retained ribbon trail" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    outResult->executedRibbonTrailRemoveCommands += 1;
    outResult->renderedAny = true;
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
