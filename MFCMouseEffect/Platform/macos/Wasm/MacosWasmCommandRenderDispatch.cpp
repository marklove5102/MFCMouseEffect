#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.h"
#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include <cstring>

namespace mousefx::platform::macos::wasm_render_dispatch {

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
    case mousefx::wasm::CommandKind::SpawnText:
        return HandleSpawnTextCommand(raw, config, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnImage:
        return HandleSpawnImageCommand(raw, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnImageAffine:
        return HandleSpawnImageAffineCommand(raw, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnPulse:
        return HandleSpawnPulseCommand(raw, config, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnPolyline:
        return HandleSpawnPolylineCommand(raw, record.sizeBytes, config, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnPathStroke:
        return HandleSpawnPathStrokeCommand(raw, record.sizeBytes, config, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnPathFill:
        return HandleSpawnPathFillCommand(raw, record.sizeBytes, config, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnRibbonStrip:
        return HandleSpawnRibbonStripCommand(raw, record.sizeBytes, config, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnGlowBatch:
        return HandleSpawnGlowBatchCommand(raw, record.sizeBytes, config, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnSpriteBatch:
        return HandleSpawnSpriteBatchCommand(raw, record.sizeBytes, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::SpawnQuadBatch:
        return HandleSpawnQuadBatchCommand(raw, record.sizeBytes, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertGlowEmitter:
        return HandleUpsertGlowEmitterCommand(raw, record.sizeBytes, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::RemoveGlowEmitter:
        return HandleRemoveGlowEmitterCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertSpriteEmitter:
        return HandleUpsertSpriteEmitterCommand(raw, record.sizeBytes, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::RemoveSpriteEmitter:
        return HandleRemoveSpriteEmitterCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertParticleEmitter:
        return HandleUpsertParticleEmitterCommand(raw, record.sizeBytes, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::RemoveParticleEmitter:
        return HandleRemoveParticleEmitterCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertRibbonTrail:
        return HandleUpsertRibbonTrailCommand(raw, record.sizeBytes, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::RemoveRibbonTrail:
        return HandleRemoveRibbonTrailCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertQuadField:
        return HandleUpsertQuadFieldCommand(raw, record.sizeBytes, config, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::RemoveQuadField:
        return HandleRemoveQuadFieldCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::RemoveGroup:
        return HandleRemoveGroupCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertGroupPresentation:
        return HandleUpsertGroupPresentationCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertGroupClipRect:
        return HandleUpsertGroupClipRectCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertGroupLayer:
        return HandleUpsertGroupLayerCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertGroupTransform:
        return HandleUpsertGroupTransformCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertGroupLocalOrigin:
        return HandleUpsertGroupLocalOriginCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertGroupMaterial:
        return HandleUpsertGroupMaterialCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
    case mousefx::wasm::CommandKind::UpsertGroupPass:
        return HandleUpsertGroupPassCommand(raw, record.sizeBytes, activeManifestPath, outResult, outThrottleCounters);
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
