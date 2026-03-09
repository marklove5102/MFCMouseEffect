#pragma once

#include <cstdint>
#include <string>

#include "MouseFx/Core/Wasm/WasmParticleEmitterCommandConfig.h"
#include "MouseFx/Interfaces/RenderSemantics.h"

namespace mousefx::wasm {

struct RetainedParticleEmitterRuntimeCounters final {
    uint64_t upsertRequests = 0;
    uint64_t removeRequests = 0;
    uint64_t activeEmitters = 0;
};

bool UpsertRetainedParticleEmitter(
    const std::wstring& activeManifestPath,
    const ResolvedParticleEmitterCommand& resolved,
    std::string* outError);

bool RemoveRetainedParticleEmitter(
    const std::wstring& activeManifestPath,
    uint32_t emitterId,
    std::string* outError);
uint32_t RemoveRetainedParticleEmittersByGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId);
void ApplyRetainedParticleEmitterGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float alphaMultiplier,
    bool visible);
void ApplyRetainedParticleEmitterGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    const mousefx::RenderClipRect& groupClipRect);
void ApplyRetainedParticleEmitterGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasBlendOverride,
    mousefx::RenderBlendMode blendMode,
    int32_t sortBias);
void ApplyRetainedParticleEmitterGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float offsetXPx,
    float offsetYPx,
    float rotationRad,
    float uniformScale);
void ApplyRetainedParticleEmitterGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float originXPx,
    float originYPx);
void ApplyRetainedParticleEmitterGroupMaterial(
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
void ApplyRetainedParticleEmitterGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    uint8_t passKind,
    float passAmount,
    float responseAmount,
    uint8_t passMode,
    float phaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff);

void ResetRetainedParticleEmittersForManifest(const std::wstring& activeManifestPath);
void ResetAllRetainedParticleEmitters();
RetainedParticleEmitterRuntimeCounters GetRetainedParticleEmitterRuntimeCounters();

} // namespace mousefx::wasm
