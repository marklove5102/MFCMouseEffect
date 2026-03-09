#pragma once

#include <cstdint>
#include <string>

#include "MouseFx/Core/Wasm/WasmGlowEmitterCommandConfig.h"
#include "MouseFx/Interfaces/RenderSemantics.h"

namespace mousefx::wasm {

struct RetainedGlowEmitterRuntimeCounters final {
    uint64_t upsertRequests = 0;
    uint64_t removeRequests = 0;
    uint64_t activeEmitters = 0;
};

bool UpsertRetainedGlowEmitter(
    const std::wstring& activeManifestPath,
    const ResolvedGlowEmitterCommand& resolved,
    std::string* outError);

bool RemoveRetainedGlowEmitter(
    const std::wstring& activeManifestPath,
    uint32_t emitterId,
    std::string* outError);
uint32_t RemoveRetainedGlowEmittersByGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId);
void ApplyRetainedGlowEmitterGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float alphaMultiplier,
    bool visible);
void ApplyRetainedGlowEmitterGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    const mousefx::RenderClipRect& groupClipRect);
void ApplyRetainedGlowEmitterGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasBlendOverride,
    mousefx::RenderBlendMode blendMode,
    int32_t sortBias);
void ApplyRetainedGlowEmitterGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float offsetXPx,
    float offsetYPx,
    float rotationRad,
    float uniformScale);
void ApplyRetainedGlowEmitterGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float originXPx,
    float originYPx);
void ApplyRetainedGlowEmitterGroupMaterial(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasTintOverride,
    uint32_t tintArgb,
    float intensityMultiplier,
    uint8_t styleKind,
    float styleAmount,
    float diffusionAmount,
    float persistenceAmount,
    float echoAmount,
    float echoDriftPx,
    uint8_t feedbackMode,
    float feedbackPhaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff);
void ApplyRetainedGlowEmitterGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    uint8_t passKind,
    float passAmount,
    float responseAmount,
    uint8_t passMode,
    float phaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff);

void ResetRetainedGlowEmittersForManifest(const std::wstring& activeManifestPath);
void ResetAllRetainedGlowEmitters();
RetainedGlowEmitterRuntimeCounters GetRetainedGlowEmitterRuntimeCounters();

} // namespace mousefx::wasm
