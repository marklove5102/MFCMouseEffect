#pragma once

#include <cstdint>
#include <string>

#include "MouseFx/Core/Wasm/WasmRibbonTrailCommandConfig.h"
#include "MouseFx/Interfaces/RenderSemantics.h"

namespace mousefx::wasm {

struct RetainedRibbonTrailRuntimeCounters final {
    uint64_t upsertRequests = 0;
    uint64_t removeRequests = 0;
    uint64_t activeTrails = 0;
};

bool UpsertRetainedRibbonTrail(
    const std::wstring& activeManifestPath,
    const ResolvedRibbonTrailCommand& resolved,
    std::string* outError);

bool RemoveRetainedRibbonTrail(
    const std::wstring& activeManifestPath,
    uint32_t trailId,
    std::string* outError);
uint32_t RemoveRetainedRibbonTrailsByGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId);
void ApplyRetainedRibbonTrailGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float alphaMultiplier,
    bool visible);
void ApplyRetainedRibbonTrailGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    const mousefx::RenderClipRect& groupClipRect);
void ApplyRetainedRibbonTrailGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasBlendOverride,
    mousefx::RenderBlendMode blendMode,
    int32_t sortBias);
void ApplyRetainedRibbonTrailGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float offsetXPx,
    float offsetYPx,
    float rotationRad,
    float uniformScale);
void ApplyRetainedRibbonTrailGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float originXPx,
    float originYPx);
void ApplyRetainedRibbonTrailGroupMaterial(
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
void ApplyRetainedRibbonTrailGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    uint8_t passKind,
    float passAmount,
    float responseAmount,
    uint8_t passMode,
    float phaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff);

void ResetRetainedRibbonTrailsForManifest(const std::wstring& activeManifestPath);
void ResetAllRetainedRibbonTrails();
RetainedRibbonTrailRuntimeCounters GetRetainedRibbonTrailRuntimeCounters();

} // namespace mousefx::wasm
