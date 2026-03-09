#pragma once

#include <cstdint>
#include <string>

#include "MouseFx/Core/Wasm/WasmSpriteEmitterCommandConfig.h"
#include "MouseFx/Interfaces/RenderSemantics.h"

namespace mousefx::wasm {

struct RetainedSpriteEmitterRuntimeCounters final {
    uint64_t upsertRequests = 0;
    uint64_t removeRequests = 0;
    uint64_t activeEmitters = 0;
};

bool UpsertRetainedSpriteEmitter(
    const std::wstring& activeManifestPath,
    const ResolvedSpriteEmitterCommand& resolved,
    std::string* outError);

bool RemoveRetainedSpriteEmitter(
    const std::wstring& activeManifestPath,
    uint32_t emitterId,
    std::string* outError);
uint32_t RemoveRetainedSpriteEmittersByGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId);
void ApplyRetainedSpriteEmitterGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float alphaMultiplier,
    bool visible);
void ApplyRetainedSpriteEmitterGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    const mousefx::RenderClipRect& groupClipRect);
void ApplyRetainedSpriteEmitterGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasBlendOverride,
    mousefx::RenderBlendMode blendMode,
    int32_t sortBias);
void ApplyRetainedSpriteEmitterGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float offsetXPx,
    float offsetYPx,
    float rotationRad,
    float uniformScale);
void ApplyRetainedSpriteEmitterGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float originXPx,
    float originYPx);
void ApplyRetainedSpriteEmitterGroupMaterial(
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
void ApplyRetainedSpriteEmitterGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    uint8_t passKind,
    float passAmount,
    float responseAmount,
    uint8_t passMode,
    float phaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff);

void ResetRetainedSpriteEmittersForManifest(const std::wstring& activeManifestPath);
void ResetAllRetainedSpriteEmitters();
RetainedSpriteEmitterRuntimeCounters GetRetainedSpriteEmitterRuntimeCounters();

} // namespace mousefx::wasm
