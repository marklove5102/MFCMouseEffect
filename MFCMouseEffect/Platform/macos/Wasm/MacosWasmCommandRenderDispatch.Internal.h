#pragma once

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.h"
#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

namespace mousefx::platform::macos::wasm_render_dispatch {

bool AccountThrottle(
    WasmOverlayRenderResult renderResult,
    bool isText,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnTextCommand(
    const uint8_t* raw,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnImageCommand(
    const uint8_t* raw,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnImageAffineCommand(
    const uint8_t* raw,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnPulseCommand(
    const uint8_t* raw,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnPolylineCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnPathStrokeCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnPathFillCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnRibbonStripCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnGlowBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnSpriteBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleSpawnQuadBatchCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleUpsertGlowEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleRemoveGlowEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleUpsertSpriteEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleRemoveSpriteEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleUpsertParticleEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleRemoveParticleEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleUpsertRibbonTrailCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleRemoveRibbonTrailCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleUpsertQuadFieldCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const mousefx::EffectConfig& config,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleRemoveQuadFieldCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

bool HandleRemoveGroupCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);
bool HandleUpsertGroupPresentationCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);
bool HandleUpsertGroupClipRectCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);
bool HandleUpsertGroupLayerCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);
bool HandleUpsertGroupTransformCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);
bool HandleUpsertGroupLocalOriginCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);
bool HandleUpsertGroupMaterialCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);
bool HandleUpsertGroupPassCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters);

} // namespace mousefx::platform::macos::wasm_render_dispatch
