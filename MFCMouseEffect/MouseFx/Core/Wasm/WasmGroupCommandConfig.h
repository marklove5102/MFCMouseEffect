#pragma once

#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
#include "MouseFx/Core/Wasm/WasmGroupPassRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialRuntime.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"
#include "MouseFx/Interfaces/RenderSemantics.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace mousefx::wasm {

struct ResolvedGroupPresentationCommand final {
    uint32_t groupId = 0u;
    float alphaMultiplier = 1.0f;
    bool visible = true;
};

struct ResolvedGroupClipRectCommand final {
    uint32_t groupId = 0u;
    mousefx::RenderClipRect clipRect{};
    bool enabled = false;
    uint8_t maskShapeKind = kGroupClipMaskShapeRect;
    float cornerRadiusPx = 0.0f;
};

struct ResolvedGroupLayerCommand final {
    uint32_t groupId = 0u;
    bool hasBlendOverride = false;
    mousefx::RenderBlendMode blendMode = mousefx::RenderBlendMode::Normal;
    int32_t sortBias = 0;
};

struct ResolvedGroupTransformCommand final {
    uint32_t groupId = 0u;
    float offsetXPx = 0.0f;
    float offsetYPx = 0.0f;
    float rotationRad = 0.0f;
    float uniformScale = 1.0f;
    float pivotXPx = 0.0f;
    float pivotYPx = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct ResolvedGroupLocalOriginCommand final {
    uint32_t groupId = 0u;
    float originXPx = 0.0f;
    float originYPx = 0.0f;
};

struct ResolvedGroupMaterialCommand final {
    uint32_t groupId = 0u;
    bool hasTintOverride = false;
    uint32_t tintArgb = 0xFFFFFFFFu;
    float intensityMultiplier = 1.0f;
    uint8_t styleKind = kGroupMaterialStyleNone;
    float styleAmount = 0.0f;
    float diffusionAmount = 0.0f;
    float persistenceAmount = 0.0f;
    float echoAmount = 0.0f;
    float echoDriftPx = 0.0f;
    uint8_t feedbackMode = kGroupMaterialFeedbackModeDirectional;
    float feedbackPhaseRad = 0.0f;
    uint8_t feedbackLayerCount = 1u;
    float feedbackLayerFalloff = 0.5f;
};

struct ResolvedGroupPassCommand final {
    uint32_t groupId = 0u;
    uint8_t passKind = kGroupPassKindNone;
    float passAmount = 0.0f;
    float responseAmount = 0.0f;
    GroupPassStageState secondaryStage{};
    GroupPassStageState tertiaryStage{};
    uint8_t passMode = kGroupPassModeDirectional;
    float phaseRad = 0.0f;
    uint8_t feedbackLayerCount = 1u;
    float feedbackLayerFalloff = 0.5f;
};

template <typename TTail>
inline bool TryReadOptionalGroupTail(
    const uint8_t* raw,
    size_t sizeBytes,
    size_t offsetBytes,
    TTail* outTail) {
    if (!raw || !outTail) {
        return false;
    }
    if (sizeBytes < offsetBytes + sizeof(TTail)) {
        return false;
    }
    std::memcpy(outTail, raw + offsetBytes, sizeof(TTail));
    return true;
}

inline bool TryResolveUpsertGroupPresentationCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    ResolvedGroupPresentationCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_group_presentation command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertGroupPresentationCommandV1)) {
        if (outError) {
            *outError = "upsert_group_presentation command truncated";
        }
        return false;
    }

    UpsertGroupPresentationCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.groupId == 0u) {
        if (outError) {
            *outError = "upsert_group_presentation requires non-zero group_id";
        }
        return false;
    }

    ResolvedGroupPresentationCommand resolved{};
    resolved.groupId = cmd.groupId;
    resolved.alphaMultiplier = std::clamp(cmd.alphaMultiplier, 0.0f, 1.0f);
    resolved.visible = (cmd.flags & kUpsertGroupPresentationFlagVisible) != 0u;
    *outResolved = resolved;
    return true;
}

inline bool TryResolveUpsertGroupClipRectCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    ResolvedGroupClipRectCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_group_clip_rect command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertGroupClipRectCommandV1)) {
        if (outError) {
            *outError = "upsert_group_clip_rect command truncated";
        }
        return false;
    }

    UpsertGroupClipRectCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.groupId == 0u) {
        if (outError) {
            *outError = "upsert_group_clip_rect requires non-zero group_id";
        }
        return false;
    }

    ResolvedGroupClipRectCommand resolved{};
    resolved.groupId = cmd.groupId;
    resolved.enabled = (cmd.flags & kUpsertGroupClipRectFlagEnabled) != 0u &&
        cmd.widthPx > 0.0f &&
        cmd.heightPx > 0.0f;
    if (sizeBytes >= sizeof(UpsertGroupClipRectCommandV1) + sizeof(GroupClipMaskTailV1)) {
        GroupClipMaskTailV1 tail{};
        std::memcpy(
            &tail,
            raw + sizeof(UpsertGroupClipRectCommandV1),
            sizeof(GroupClipMaskTailV1));
        if (tail.shapeKind <= kGroupClipMaskShapeEllipse) {
            resolved.maskShapeKind = tail.shapeKind;
        }
        resolved.cornerRadiusPx = std::max(0.0f, tail.cornerRadiusPx);
    }
    if (resolved.enabled) {
        resolved.clipRect.leftPx = cmd.leftPx;
        resolved.clipRect.topPx = cmd.topPx;
        resolved.clipRect.widthPx = std::max(0.0f, cmd.widthPx);
        resolved.clipRect.heightPx = std::max(0.0f, cmd.heightPx);
    }
    *outResolved = resolved;
    return true;
}

inline bool TryResolveRemoveGroupCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    uint32_t* outGroupId,
    std::string* outError) {
    if (!raw || !outGroupId) {
        if (outError) {
            *outError = "remove_group command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(RemoveGroupCommandV1)) {
        if (outError) {
            *outError = "remove_group command truncated";
        }
        return false;
    }

    RemoveGroupCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.groupId == 0u) {
        if (outError) {
            *outError = "remove_group requires non-zero group_id";
        }
        return false;
    }

    *outGroupId = cmd.groupId;
    return true;
}

inline bool TryResolveUpsertGroupLayerCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    ResolvedGroupLayerCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_group_layer command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertGroupLayerCommandV1)) {
        if (outError) {
            *outError = "upsert_group_layer command truncated";
        }
        return false;
    }

    UpsertGroupLayerCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.groupId == 0u) {
        if (outError) {
            *outError = "upsert_group_layer requires non-zero group_id";
        }
        return false;
    }

    ResolvedGroupLayerCommand resolved{};
    resolved.groupId = cmd.groupId;
    resolved.hasBlendOverride = (cmd.flags & kUpsertGroupLayerFlagBlendOverrideEnabled) != 0u;
    resolved.blendMode = ResolveRenderBlendMode(cmd.blendMode);
    resolved.sortBias = cmd.sortBias;
    *outResolved = resolved;
    return true;
}

inline bool TryResolveUpsertGroupTransformCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    ResolvedGroupTransformCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_group_transform command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertGroupTransformCommandV1)) {
        if (outError) {
            *outError = "upsert_group_transform command truncated";
        }
        return false;
    }

    UpsertGroupTransformCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.groupId == 0u) {
        if (outError) {
            *outError = "upsert_group_transform requires non-zero group_id";
        }
        return false;
    }

    ResolvedGroupTransformCommand resolved{};
    resolved.groupId = cmd.groupId;
    resolved.offsetXPx = cmd.offsetXPx;
    resolved.offsetYPx = cmd.offsetYPx;
    resolved.scaleX = 1.0f;
    resolved.scaleY = 1.0f;
    if (sizeBytes >= sizeof(UpsertGroupTransformCommandV1) + sizeof(GroupTransformTailV1)) {
        GroupTransformTailV1 tail{};
        std::memcpy(&tail, raw + sizeof(UpsertGroupTransformCommandV1), sizeof(tail));
        resolved.rotationRad = std::clamp(tail.rotationRad, -6.2831853f, 6.2831853f);
        resolved.uniformScale = std::clamp(tail.uniformScale, 0.05f, 8.0f);
        resolved.scaleX = resolved.uniformScale;
        resolved.scaleY = resolved.uniformScale;
    }
    if (sizeBytes >= sizeof(UpsertGroupTransformCommandV1) + sizeof(GroupTransformTailV1) + sizeof(GroupTransformPivotTailV1)) {
        GroupTransformPivotTailV1 pivotTail{};
        std::memcpy(
            &pivotTail,
            raw + sizeof(UpsertGroupTransformCommandV1) + sizeof(GroupTransformTailV1),
            sizeof(GroupTransformPivotTailV1));
        resolved.pivotXPx = std::clamp(pivotTail.pivotXPx, -4096.0f, 4096.0f);
        resolved.pivotYPx = std::clamp(pivotTail.pivotYPx, -4096.0f, 4096.0f);
    }
    if (sizeBytes >= sizeof(UpsertGroupTransformCommandV1) + sizeof(GroupTransformTailV1) + sizeof(GroupTransformPivotTailV1) + sizeof(GroupTransformScale2DTailV1)) {
        GroupTransformScale2DTailV1 scaleTail{};
        std::memcpy(
            &scaleTail,
            raw + sizeof(UpsertGroupTransformCommandV1) + sizeof(GroupTransformTailV1) + sizeof(GroupTransformPivotTailV1),
            sizeof(GroupTransformScale2DTailV1));
        resolved.scaleX = std::clamp(scaleTail.scaleX, 0.05f, 8.0f);
        resolved.scaleY = std::clamp(scaleTail.scaleY, 0.05f, 8.0f);
    }
    *outResolved = resolved;
    return true;
}

inline bool TryResolveUpsertGroupMaterialCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    ResolvedGroupMaterialCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_group_material command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertGroupMaterialCommandV1)) {
        if (outError) {
            *outError = "upsert_group_material command truncated";
        }
        return false;
    }

    UpsertGroupMaterialCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.groupId == 0u) {
        if (outError) {
            *outError = "upsert_group_material requires non-zero group_id";
        }
        return false;
    }

    ResolvedGroupMaterialCommand resolved{};
    resolved.groupId = cmd.groupId;
    resolved.hasTintOverride = (cmd.flags & kUpsertGroupMaterialFlagTintEnabled) != 0u;
    resolved.tintArgb = cmd.tintArgb;
    resolved.intensityMultiplier = std::clamp(cmd.intensityMultiplier, 0.0f, 2.0f);
    if (sizeBytes >= sizeof(UpsertGroupMaterialCommandV1) + sizeof(GroupMaterialStyleTailV1)) {
        GroupMaterialStyleTailV1 tail{};
        std::memcpy(
            &tail,
            raw + sizeof(UpsertGroupMaterialCommandV1),
            sizeof(GroupMaterialStyleTailV1));
        resolved.styleKind = ResolveGroupMaterialStyleKind(tail.styleKind);
        resolved.styleAmount = ClampGroupMaterialStyleAmount(tail.styleAmount);
    }
    if (sizeBytes >=
        sizeof(UpsertGroupMaterialCommandV1) +
            sizeof(GroupMaterialStyleTailV1) +
            sizeof(GroupMaterialResponseTailV1)) {
        GroupMaterialResponseTailV1 tail{};
        std::memcpy(
            &tail,
            raw + sizeof(UpsertGroupMaterialCommandV1) + sizeof(GroupMaterialStyleTailV1),
            sizeof(GroupMaterialResponseTailV1));
        resolved.diffusionAmount = ClampGroupMaterialResponseAmount(tail.diffusionAmount);
        resolved.persistenceAmount = ClampGroupMaterialResponseAmount(tail.persistenceAmount);
    }
    if (sizeBytes >=
        sizeof(UpsertGroupMaterialCommandV1) +
            sizeof(GroupMaterialStyleTailV1) +
            sizeof(GroupMaterialResponseTailV1) +
            sizeof(GroupMaterialFeedbackTailV1)) {
        GroupMaterialFeedbackTailV1 tail{};
        std::memcpy(
            &tail,
            raw + sizeof(UpsertGroupMaterialCommandV1) + sizeof(GroupMaterialStyleTailV1) + sizeof(GroupMaterialResponseTailV1),
            sizeof(GroupMaterialFeedbackTailV1));
        resolved.echoAmount = ClampGroupMaterialEchoAmount(tail.echoAmount);
        resolved.echoDriftPx = ClampGroupMaterialEchoDriftPx(tail.echoDriftPx);
    }
    if (sizeBytes >=
        sizeof(UpsertGroupMaterialCommandV1) +
            sizeof(GroupMaterialStyleTailV1) +
            sizeof(GroupMaterialResponseTailV1) +
            sizeof(GroupMaterialFeedbackTailV1) +
            sizeof(GroupMaterialFeedbackModeTailV1)) {
        GroupMaterialFeedbackModeTailV1 tail{};
        std::memcpy(
            &tail,
            raw + sizeof(UpsertGroupMaterialCommandV1) +
                sizeof(GroupMaterialStyleTailV1) +
                sizeof(GroupMaterialResponseTailV1) +
                sizeof(GroupMaterialFeedbackTailV1),
            sizeof(GroupMaterialFeedbackModeTailV1));
        resolved.feedbackMode = ResolveGroupMaterialFeedbackMode(tail.feedbackMode);
        resolved.feedbackPhaseRad = NormalizeGroupMaterialPhaseRad(tail.phaseRad);
    }
    if (sizeBytes >=
        sizeof(UpsertGroupMaterialCommandV1) +
            sizeof(GroupMaterialStyleTailV1) +
            sizeof(GroupMaterialResponseTailV1) +
            sizeof(GroupMaterialFeedbackTailV1) +
            sizeof(GroupMaterialFeedbackModeTailV1) +
            sizeof(GroupMaterialFeedbackStackTailV1)) {
        GroupMaterialFeedbackStackTailV1 tail{};
        std::memcpy(
            &tail,
            raw + sizeof(UpsertGroupMaterialCommandV1) +
                sizeof(GroupMaterialStyleTailV1) +
                sizeof(GroupMaterialResponseTailV1) +
                sizeof(GroupMaterialFeedbackTailV1) +
                sizeof(GroupMaterialFeedbackModeTailV1),
            sizeof(GroupMaterialFeedbackStackTailV1));
        resolved.feedbackLayerCount = ClampGroupMaterialFeedbackLayerCount(tail.layerCount);
        resolved.feedbackLayerFalloff = ClampGroupMaterialFeedbackLayerFalloff(tail.layerFalloff);
    }
    *outResolved = resolved;
    return true;
}

inline bool TryResolveUpsertGroupLocalOriginCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    ResolvedGroupLocalOriginCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_group_local_origin command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertGroupLocalOriginCommandV1)) {
        if (outError) {
            *outError = "upsert_group_local_origin command truncated";
        }
        return false;
    }

    UpsertGroupLocalOriginCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.groupId == 0u) {
        if (outError) {
            *outError = "upsert_group_local_origin requires non-zero group_id";
        }
        return false;
    }

    ResolvedGroupLocalOriginCommand resolved{};
    resolved.groupId = cmd.groupId;
    resolved.originXPx = cmd.originXPx;
    resolved.originYPx = cmd.originYPx;
    *outResolved = resolved;
    return true;
}

inline bool TryResolveUpsertGroupPassCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    ResolvedGroupPassCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_group_pass command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertGroupPassCommandV1)) {
        if (outError) {
            *outError = "upsert_group_pass command truncated";
        }
        return false;
    }

    UpsertGroupPassCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.groupId == 0u) {
        if (outError) {
            *outError = "upsert_group_pass requires non-zero group_id";
        }
        return false;
    }

    ResolvedGroupPassCommand resolved{};
    resolved.groupId = cmd.groupId;
    resolved.passKind = ResolveGroupPassKind(cmd.passKind);
    resolved.passAmount = ClampGroupPassAmount(cmd.passAmount);
    resolved.responseAmount = ClampGroupPassResponseAmount(cmd.responseAmount);
    size_t groupPassTailOffset = sizeof(UpsertGroupPassCommandV1);
    {
        GroupPassModeTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.passMode = ResolveGroupPassMode(tail.passMode);
            resolved.phaseRad = NormalizeGroupPassPhaseRad(tail.phaseRad);
        }
        groupPassTailOffset += sizeof(GroupPassModeTailV1);
    }
    {
        GroupPassStackTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.feedbackLayerCount = ClampGroupPassFeedbackLayerCount(tail.layerCount);
            resolved.feedbackLayerFalloff = ClampGroupPassFeedbackLayerFalloff(tail.layerFalloff);
        }
        groupPassTailOffset += sizeof(GroupPassStackTailV1);
    }
    {
        GroupPassPipelineTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.secondaryStage.passKind = ResolveGroupPassKind(tail.secondaryPassKind);
            resolved.secondaryStage.passAmount = ClampGroupPassAmount(tail.secondaryPassAmount);
            resolved.secondaryStage.responseAmount = ClampGroupPassResponseAmount(tail.secondaryResponseAmount);
        }
        groupPassTailOffset += sizeof(GroupPassPipelineTailV1);
    }
    {
        GroupPassBlendTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.secondaryStage.blendMode = ResolveGroupPassBlendMode(tail.blendMode);
            resolved.secondaryStage.blendWeight = ClampGroupPassBlendWeight(tail.blendWeight);
        }
        groupPassTailOffset += sizeof(GroupPassBlendTailV1);
    }
    {
        GroupPassRoutingTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.secondaryStage.routeMask = ResolveGroupPassRouteMask(tail.routeMask);
        }
        groupPassTailOffset += sizeof(GroupPassRoutingTailV1);
    }
    {
        GroupPassLaneResponseTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.secondaryStage.glowResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.glowResponse);
            resolved.secondaryStage.spriteResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.spriteResponse);
            resolved.secondaryStage.particleResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.particleResponse);
            resolved.secondaryStage.ribbonResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.ribbonResponse);
            resolved.secondaryStage.quadResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.quadResponse);
        }
        groupPassTailOffset += sizeof(GroupPassLaneResponseTailV1);
    }
    {
        GroupPassTemporalTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.secondaryStage.phaseRateRadPerSec = ClampGroupPassPhaseRateRadPerSec(tail.phaseRateRadPerSec);
            resolved.secondaryStage.decayPerSec = ClampGroupPassDecayPerSec(tail.secondaryDecayPerSec);
            resolved.secondaryStage.decayFloor = ClampGroupPassDecayFloor(tail.secondaryDecayFloor);
        }
        groupPassTailOffset += sizeof(GroupPassTemporalTailV1);
    }
    {
        GroupPassTemporalModeTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.secondaryStage.temporalMode = ResolveGroupPassTemporalMode(tail.temporalMode);
            resolved.secondaryStage.temporalStrength = ClampGroupPassTemporalStrength(tail.temporalStrength);
        }
        groupPassTailOffset += sizeof(GroupPassTemporalModeTailV1);
    }
    {
        GroupPassTertiaryTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.tertiaryStage.passKind = ResolveGroupPassKind(tail.tertiaryPassKind);
            resolved.tertiaryStage.passAmount = ClampGroupPassAmount(tail.tertiaryPassAmount);
            resolved.tertiaryStage.responseAmount = ClampGroupPassResponseAmount(tail.tertiaryResponseAmount);
            resolved.tertiaryStage.blendMode = ResolveGroupPassBlendMode(tail.tertiaryBlendMode);
            resolved.tertiaryStage.blendWeight = ClampGroupPassBlendWeight(tail.tertiaryBlendWeight);
        }
        groupPassTailOffset += sizeof(GroupPassTertiaryTailV1);
    }
    {
        GroupPassTertiaryRoutingTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.tertiaryStage.routeMask = ResolveGroupPassRouteMask(tail.tertiaryRouteMask);
        }
        groupPassTailOffset += sizeof(GroupPassTertiaryRoutingTailV1);
    }
    {
        GroupPassTertiaryLaneResponseTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.tertiaryStage.glowResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.glowResponse);
            resolved.tertiaryStage.spriteResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.spriteResponse);
            resolved.tertiaryStage.particleResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.particleResponse);
            resolved.tertiaryStage.ribbonResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.ribbonResponse);
            resolved.tertiaryStage.quadResponseMultiplier = ClampGroupPassLaneResponseMultiplier(tail.quadResponse);
        }
        groupPassTailOffset += sizeof(GroupPassTertiaryLaneResponseTailV1);
    }
    {
        GroupPassTertiaryTemporalTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.tertiaryStage.phaseRateRadPerSec = ClampGroupPassPhaseRateRadPerSec(tail.phaseRateRadPerSec);
            resolved.tertiaryStage.decayPerSec = ClampGroupPassDecayPerSec(tail.tertiaryDecayPerSec);
            resolved.tertiaryStage.decayFloor = ClampGroupPassDecayFloor(tail.tertiaryDecayFloor);
        }
        groupPassTailOffset += sizeof(GroupPassTertiaryTemporalTailV1);
    }
    {
        GroupPassTertiaryTemporalModeTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.tertiaryStage.temporalMode = ResolveGroupPassTemporalMode(tail.temporalMode);
            resolved.tertiaryStage.temporalStrength = ClampGroupPassTemporalStrength(tail.temporalStrength);
        }
        groupPassTailOffset += sizeof(GroupPassTertiaryTemporalModeTailV1);
    }
    {
        GroupPassTertiaryStackTailV1 tail{};
        if (TryReadOptionalGroupTail(raw, sizeBytes, groupPassTailOffset, &tail)) {
            resolved.tertiaryStage.feedbackLayerCount = ClampGroupPassFeedbackLayerCount(tail.layerCount);
            resolved.tertiaryStage.feedbackLayerFalloff = ClampGroupPassFeedbackLayerFalloff(tail.layerFalloff);
        }
    }
    *outResolved = resolved;
    return true;
}

} // namespace mousefx::wasm
