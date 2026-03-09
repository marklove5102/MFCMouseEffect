#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmCommandRenderDispatch.Internal.h"

#include "MouseFx/Core/Wasm/WasmGroupCommandConfig.h"
#include "MouseFx/Core/Wasm/WasmGroupClipRectRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupLocalOriginRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupPassRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupLayerRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupPresentationRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupTransformRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedGlowEmitterRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedParticleEmitterRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedQuadFieldRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedRibbonTrailRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedSpriteEmitterRuntime.h"

namespace mousefx::platform::macos::wasm_render_dispatch {

bool HandleRemoveGroupCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    uint32_t groupId = 0u;
    std::string error;
    if (!mousefx::wasm::TryResolveRemoveGroupCommand(raw, sizeBytes, &groupId, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve remove_group command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    mousefx::wasm::RemoveGroupPresentation(activeManifestPath, groupId);
    mousefx::wasm::RemoveGroupClipRect(activeManifestPath, groupId);
    mousefx::wasm::RemoveGroupLayer(activeManifestPath, groupId);
    mousefx::wasm::RemoveGroupLocalOrigin(activeManifestPath, groupId);
    mousefx::wasm::RemoveGroupTransform(activeManifestPath, groupId);
    mousefx::wasm::RemoveGroupMaterial(activeManifestPath, groupId);
    mousefx::wasm::RemoveGroupPass(activeManifestPath, groupId);
    mousefx::wasm::RemoveRetainedGlowEmittersByGroup(activeManifestPath, groupId);
    mousefx::wasm::RemoveRetainedSpriteEmittersByGroup(activeManifestPath, groupId);
    mousefx::wasm::RemoveRetainedParticleEmittersByGroup(activeManifestPath, groupId);
    mousefx::wasm::RemoveRetainedRibbonTrailsByGroup(activeManifestPath, groupId);
    mousefx::wasm::RemoveRetainedQuadFieldsByGroup(activeManifestPath, groupId);

    outResult->executedGroupRemoveCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleUpsertGroupPresentationCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    mousefx::wasm::ResolvedGroupPresentationCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertGroupPresentationCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_presentation command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    mousefx::wasm::UpsertGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    mousefx::wasm::ApplyRetainedGlowEmitterGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    mousefx::wasm::ApplyRetainedSpriteEmitterGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    mousefx::wasm::ApplyRetainedParticleEmitterGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    mousefx::wasm::ApplyRetainedRibbonTrailGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);
    mousefx::wasm::ApplyRetainedQuadFieldGroupPresentation(
        activeManifestPath,
        resolved.groupId,
        resolved.alphaMultiplier,
        resolved.visible);

    outResult->executedGroupPresentationCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleUpsertGroupClipRectCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    mousefx::wasm::ResolvedGroupClipRectCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertGroupClipRectCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_clip_rect command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    if (resolved.enabled) {
        mousefx::wasm::UpsertGroupClipRect(
            activeManifestPath,
            resolved.groupId,
            resolved.clipRect,
            resolved.maskShapeKind,
            resolved.cornerRadiusPx);
    } else {
        mousefx::wasm::RemoveGroupClipRect(activeManifestPath, resolved.groupId);
    }
    mousefx::wasm::ApplyRetainedGlowEmitterGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);
    mousefx::wasm::ApplyRetainedSpriteEmitterGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);
    mousefx::wasm::ApplyRetainedParticleEmitterGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);
    mousefx::wasm::ApplyRetainedRibbonTrailGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);
    mousefx::wasm::ApplyRetainedQuadFieldGroupClipRect(activeManifestPath, resolved.groupId, resolved.clipRect);

    outResult->executedGroupClipRectCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleUpsertGroupLayerCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    mousefx::wasm::ResolvedGroupLayerCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertGroupLayerCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_layer command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    mousefx::wasm::UpsertGroupLayer(
        activeManifestPath,
        resolved.groupId,
        resolved.hasBlendOverride,
        resolved.blendMode,
        resolved.sortBias);
    mousefx::wasm::ApplyRetainedGlowEmitterGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);
    mousefx::wasm::ApplyRetainedSpriteEmitterGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);
    mousefx::wasm::ApplyRetainedParticleEmitterGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);
    mousefx::wasm::ApplyRetainedRibbonTrailGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);
    mousefx::wasm::ApplyRetainedQuadFieldGroupLayer(activeManifestPath, resolved.groupId, resolved.hasBlendOverride, resolved.blendMode, resolved.sortBias);

    outResult->executedGroupLayerCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleUpsertGroupTransformCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    mousefx::wasm::ResolvedGroupTransformCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertGroupTransformCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_transform command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    mousefx::wasm::UpsertGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale,
        resolved.pivotXPx,
        resolved.pivotYPx,
        resolved.scaleX,
        resolved.scaleY);
    mousefx::wasm::ApplyRetainedGlowEmitterGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);
    mousefx::wasm::ApplyRetainedSpriteEmitterGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);
    mousefx::wasm::ApplyRetainedParticleEmitterGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);
    mousefx::wasm::ApplyRetainedRibbonTrailGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);
    mousefx::wasm::ApplyRetainedQuadFieldGroupTransform(
        activeManifestPath,
        resolved.groupId,
        resolved.offsetXPx,
        resolved.offsetYPx,
        resolved.rotationRad,
        resolved.uniformScale);

    outResult->executedGroupTransformCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleUpsertGroupLocalOriginCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    mousefx::wasm::ResolvedGroupLocalOriginCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertGroupLocalOriginCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_local_origin command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    mousefx::wasm::UpsertGroupLocalOrigin(
        activeManifestPath,
        resolved.groupId,
        resolved.originXPx,
        resolved.originYPx);
    mousefx::wasm::ApplyRetainedGlowEmitterGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);
    mousefx::wasm::ApplyRetainedSpriteEmitterGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);
    mousefx::wasm::ApplyRetainedParticleEmitterGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);
    mousefx::wasm::ApplyRetainedRibbonTrailGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);
    mousefx::wasm::ApplyRetainedQuadFieldGroupLocalOrigin(activeManifestPath, resolved.groupId, resolved.originXPx, resolved.originYPx);

    outResult->executedGroupLocalOriginCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleUpsertGroupMaterialCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    mousefx::wasm::ResolvedGroupMaterialCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertGroupMaterialCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_material command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    mousefx::wasm::UpsertGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedGlowEmitterGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedSpriteEmitterGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedParticleEmitterGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedRibbonTrailGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedQuadFieldGroupMaterial(
        activeManifestPath,
        resolved.groupId,
        resolved.hasTintOverride,
        resolved.tintArgb,
        resolved.intensityMultiplier,
        resolved.styleKind,
        resolved.styleAmount,
        resolved.diffusionAmount,
        resolved.persistenceAmount,
        resolved.echoAmount,
        resolved.echoDriftPx,
        resolved.feedbackMode,
        resolved.feedbackPhaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);

    outResult->executedGroupMaterialCommands += 1;
    outResult->renderedAny = true;
    return true;
}

bool HandleUpsertGroupPassCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const std::wstring& activeManifestPath,
    mousefx::wasm::CommandExecutionResult* outResult,
    ThrottleCounters* outThrottleCounters) {
    (void)outThrottleCounters;
    if (!outResult) {
        return false;
    }

    mousefx::wasm::ResolvedGroupPassCommand resolved{};
    std::string error;
    if (!mousefx::wasm::TryResolveUpsertGroupPassCommand(raw, sizeBytes, &resolved, &error)) {
        outResult->lastError = error.empty() ? "failed to resolve upsert_group_pass command" : error;
        outResult->droppedCommands += 1;
        return true;
    }

    mousefx::wasm::UpsertGroupPass(
        activeManifestPath,
        resolved.groupId,
        resolved.passKind,
        resolved.passAmount,
        resolved.responseAmount,
        resolved.secondaryStage,
        resolved.tertiaryStage,
        resolved.passMode,
        resolved.phaseRad,
        resolved.feedbackLayerCount,
        resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedGlowEmitterGroupPass(activeManifestPath, resolved.groupId, resolved.passKind, resolved.passAmount, resolved.responseAmount, resolved.passMode, resolved.phaseRad, resolved.feedbackLayerCount, resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedSpriteEmitterGroupPass(activeManifestPath, resolved.groupId, resolved.passKind, resolved.passAmount, resolved.responseAmount, resolved.passMode, resolved.phaseRad, resolved.feedbackLayerCount, resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedParticleEmitterGroupPass(activeManifestPath, resolved.groupId, resolved.passKind, resolved.passAmount, resolved.responseAmount, resolved.passMode, resolved.phaseRad, resolved.feedbackLayerCount, resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedRibbonTrailGroupPass(activeManifestPath, resolved.groupId, resolved.passKind, resolved.passAmount, resolved.responseAmount, resolved.passMode, resolved.phaseRad, resolved.feedbackLayerCount, resolved.feedbackLayerFalloff);
    mousefx::wasm::ApplyRetainedQuadFieldGroupPass(activeManifestPath, resolved.groupId, resolved.passKind, resolved.passAmount, resolved.responseAmount, resolved.passMode, resolved.phaseRad, resolved.feedbackLayerCount, resolved.feedbackLayerFalloff);

    outResult->executedGroupPassCommands += 1;
    outResult->renderedAny = true;
    return true;
}

} // namespace mousefx::platform::macos::wasm_render_dispatch
