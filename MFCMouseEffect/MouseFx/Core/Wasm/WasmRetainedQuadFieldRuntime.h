#pragma once

#include "MouseFx/Core/Wasm/WasmQuadFieldCommandConfig.h"
#include "MouseFx/Interfaces/RenderSemantics.h"

#include <cstdint>
#include <string>

namespace mousefx::wasm {

struct RetainedQuadFieldRuntimeCounters final {
    uint64_t upsertRequests = 0;
    uint64_t removeRequests = 0;
    uint64_t activeFields = 0;
};

bool UpsertRetainedQuadField(
    const std::wstring& activeManifestPath,
    const ResolvedQuadFieldCommand& resolved,
    std::string* outError);

bool RemoveRetainedQuadField(
    const std::wstring& activeManifestPath,
    uint32_t fieldId,
    std::string* outError);
uint32_t RemoveRetainedQuadFieldsByGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId);
void ApplyRetainedQuadFieldGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float alphaMultiplier,
    bool visible);
void ApplyRetainedQuadFieldGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    const mousefx::RenderClipRect& groupClipRect);
void ApplyRetainedQuadFieldGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasBlendOverride,
    mousefx::RenderBlendMode blendMode,
    int32_t sortBias);
void ApplyRetainedQuadFieldGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float offsetXPx,
    float offsetYPx,
    float rotationRad,
    float uniformScale);
void ApplyRetainedQuadFieldGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float originXPx,
    float originYPx);
void ApplyRetainedQuadFieldGroupMaterial(
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
void ApplyRetainedQuadFieldGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    uint8_t passKind,
    float passAmount,
    float responseAmount,
    uint8_t passMode,
    float phaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff);

void ResetRetainedQuadFieldsForManifest(const std::wstring& activeManifestPath);
void ResetAllRetainedQuadFields();
RetainedQuadFieldRuntimeCounters GetRetainedQuadFieldRuntimeCounters();

} // namespace mousefx::wasm
