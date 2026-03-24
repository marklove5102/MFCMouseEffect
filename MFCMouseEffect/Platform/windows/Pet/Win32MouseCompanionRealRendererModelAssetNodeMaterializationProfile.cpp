#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeRealizationRegistryProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeRegistryProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

float ResolveMaterializationWeight(
    const Win32MouseCompanionRealRendererModelAssetNodeRealizationRegistryProfile&
        nodeRealizationRegistryProfile,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile) {
    const float registryCoverage =
        static_cast<float>(nodeRegistryProfile.resolvedEntryCount) /
        static_cast<float>(std::max<uint32_t>(1u, nodeRegistryProfile.entryCount));
    return std::clamp(
        nodeRealizationRegistryProfile.realizationRegistryWeight * 0.77f + registryCoverage * 0.23f,
        0.0f,
        1.0f);
}

std::string ResolveMaterializationState(
    const Win32MouseCompanionRealRendererModelAssetNodeRealizationRegistryProfile&
        nodeRealizationRegistryProfile,
    uint32_t resolvedEntryCount,
    uint32_t entryCount,
    const std::string& adapterMode) {
    if (nodeRealizationRegistryProfile.realizationRegistryState == "preview_only" ||
        resolvedEntryCount == 0u) {
        return "preview_only";
    }
    if (resolvedEntryCount >= entryCount) {
        if (adapterMode == "pose_bound") {
            return "model_asset_node_materialization_bound";
        }
        if (adapterMode == "pose_unbound") {
            return "model_asset_node_materialization_pose_ready";
        }
        return "model_asset_node_materialization_ready";
    }
    return "model_asset_node_materialization_partial";
}

std::string BuildBrief(const std::string& state, uint32_t entryCount, uint32_t resolvedEntryCount) {
    char buffer[96];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s/%u/%u",
        state.empty() ? "preview_only" : state.c_str(),
        entryCount,
        resolvedEntryCount);
    return std::string(buffer);
}

std::string BuildMaterializationBrief(
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    return "body:" + std::string(nodeRegistryProfile.bodyEntry.resolved ? "node_materialization" : "stub") +
           "|head:" + std::string(nodeRegistryProfile.headEntry.resolved ? "node_materialization" : "stub") +
           "|appendage:" + std::string(nodeRegistryProfile.appendageEntry.resolved ? "node_materialization" : "stub") +
           "|overlay:" + std::string(nodeRegistryProfile.overlayEntry.resolved ? "node_materialization" : "stub") +
           "|grounding:" + std::string(nodeRegistryProfile.groundingEntry.resolved ? "node_materialization" : "stub") +
           "|adapter:" + (adapterMode.empty() ? "runtime_only" : adapterMode);
}

std::string BuildValueBrief(
    float materializationWeight,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    const float bodyWeight = materializationWeight * nodeRegistryProfile.bodyEntry.registryWeight;
    const float headWeight = materializationWeight * nodeRegistryProfile.headEntry.registryWeight;
    const float appendageWeight =
        materializationWeight * nodeRegistryProfile.appendageEntry.registryWeight;
    const float overlayWeight = materializationWeight * nodeRegistryProfile.overlayEntry.registryWeight;
    const float groundingWeight =
        materializationWeight * nodeRegistryProfile.groundingEntry.registryWeight;
    const float adapterWeight =
        adapterMode == "pose_bound" ? materializationWeight :
        (adapterMode == "pose_unbound" ? materializationWeight * 0.97f : materializationWeight * 0.89f);
    char buffer[192];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%.2f|head:%.2f|appendage:%.2f|overlay:%.2f|grounding:%.2f|adapter:%.2f",
        bodyWeight,
        headWeight,
        appendageWeight,
        overlayWeight,
        groundingWeight,
        adapterWeight);
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeMaterializationProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeRealizationRegistryProfile&
        nodeRealizationRegistryProfile,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile profile{};
    profile.entryCount = 5u;
    profile.resolvedEntryCount = nodeRegistryProfile.resolvedEntryCount;
    profile.materializationWeight =
        ResolveMaterializationWeight(nodeRealizationRegistryProfile, nodeRegistryProfile);
    profile.materializationState = ResolveMaterializationState(
        nodeRealizationRegistryProfile,
        profile.resolvedEntryCount,
        profile.entryCount,
        adapterMode);
    profile.brief =
        BuildBrief(profile.materializationState, profile.entryCount, profile.resolvedEntryCount);
    profile.materializationBrief = BuildMaterializationBrief(nodeRegistryProfile, adapterMode);
    profile.valueBrief =
        BuildValueBrief(profile.materializationWeight, nodeRegistryProfile, adapterMode);
    return profile;
}

Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeMaterializationProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    return BuildWin32MouseCompanionRealRendererModelAssetNodeMaterializationProfile(
        runtime.modelAssetNodeRealizationRegistryProfile,
        runtime.modelNodeRegistryProfile,
        runtime.sceneRuntimeAdapterMode);
}

void ApplyWin32MouseCompanionRealRendererModelAssetNodeMaterializationProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile& profile,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.bodyAnchorScale *= 1.0f + profile.materializationWeight * 0.010f;
    scene.headAnchorScale *= 1.0f + profile.materializationWeight * 0.012f;
    scene.accessoryAlphaScale *= 1.0f + profile.materializationWeight * 0.011f;
    scene.eyeHighlightAlpha = std::clamp(
        scene.eyeHighlightAlpha + profile.materializationWeight * 2.0f,
        0.0f,
        255.0f);
}

} // namespace mousefx::windows
