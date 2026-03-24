#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeBindingProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

float ResolveMaterializationRegistryWeight(
    const Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile&
        nodeMaterializationProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile) {
    const float bindingCoverage =
        static_cast<float>(nodeBindingProfile.boundEntryCount) /
        static_cast<float>(std::max<uint32_t>(1u, nodeBindingProfile.entryCount));
    return std::clamp(
        nodeMaterializationProfile.materializationWeight * 0.75f + bindingCoverage * 0.25f,
        0.0f,
        1.0f);
}

std::string ResolveMaterializationRegistryState(
    const Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile&
        nodeMaterializationProfile,
    uint32_t resolvedEntryCount,
    uint32_t entryCount,
    const std::string& adapterMode) {
    if (nodeMaterializationProfile.materializationState == "preview_only" ||
        resolvedEntryCount == 0u) {
        return "preview_only";
    }
    if (resolvedEntryCount >= entryCount) {
        if (adapterMode == "pose_bound") {
            return "model_asset_node_materialization_registry_bound";
        }
        if (adapterMode == "pose_unbound") {
            return "model_asset_node_materialization_registry_pose_ready";
        }
        return "model_asset_node_materialization_registry_ready";
    }
    return "model_asset_node_materialization_registry_partial";
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

std::string BuildRegistryBrief(
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    return "body:" + std::string(nodeBindingProfile.bodyEntry.bound ? "node_materialization_registry" : "stub") +
           "|head:" + std::string(nodeBindingProfile.headEntry.bound ? "node_materialization_registry" : "stub") +
           "|appendage:" + std::string(nodeBindingProfile.appendageEntry.bound ? "node_materialization_registry" : "stub") +
           "|overlay:" + std::string(nodeBindingProfile.overlayEntry.bound ? "node_materialization_registry" : "stub") +
           "|grounding:" + std::string(nodeBindingProfile.groundingEntry.bound ? "node_materialization_registry" : "stub") +
           "|adapter:" + (adapterMode.empty() ? "runtime_only" : adapterMode);
}

std::string BuildValueBrief(
    float materializationRegistryWeight,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    const float bodyWeight = materializationRegistryWeight * nodeBindingProfile.bodyEntry.bindWeight;
    const float headWeight = materializationRegistryWeight * nodeBindingProfile.headEntry.bindWeight;
    const float appendageWeight =
        materializationRegistryWeight * nodeBindingProfile.appendageEntry.bindWeight;
    const float overlayWeight =
        materializationRegistryWeight * nodeBindingProfile.overlayEntry.bindWeight;
    const float groundingWeight =
        materializationRegistryWeight * nodeBindingProfile.groundingEntry.bindWeight;
    const float adapterWeight =
        adapterMode == "pose_bound" ? materializationRegistryWeight :
        (adapterMode == "pose_unbound" ? materializationRegistryWeight * 0.97f : materializationRegistryWeight * 0.90f);
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

Win32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeMaterializationProfile&
        nodeMaterializationProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    Win32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile profile{};
    profile.entryCount = 5u;
    profile.resolvedEntryCount = nodeBindingProfile.boundEntryCount;
    profile.materializationRegistryWeight =
        ResolveMaterializationRegistryWeight(nodeMaterializationProfile, nodeBindingProfile);
    profile.materializationRegistryState = ResolveMaterializationRegistryState(
        nodeMaterializationProfile,
        profile.resolvedEntryCount,
        profile.entryCount,
        adapterMode);
    profile.brief = BuildBrief(
        profile.materializationRegistryState, profile.entryCount, profile.resolvedEntryCount);
    profile.registryBrief = BuildRegistryBrief(nodeBindingProfile, adapterMode);
    profile.valueBrief = BuildValueBrief(
        profile.materializationRegistryWeight, nodeBindingProfile, adapterMode);
    return profile;
}

Win32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    return BuildWin32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile(
        runtime.modelAssetNodeMaterializationProfile,
        runtime.modelNodeBindingProfile,
        runtime.sceneRuntimeAdapterMode);
}

void ApplyWin32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeMaterializationRegistryProfile& profile,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.bodyStrokeWidth *= 1.0f + profile.materializationRegistryWeight * 0.011f;
    scene.headStrokeWidth *= 1.0f + profile.materializationRegistryWeight * 0.012f;
    scene.shadowAlphaScale *= 1.0f + profile.materializationRegistryWeight * 0.012f;
    scene.poseBadgeAlpha = std::clamp(
        scene.poseBadgeAlpha + profile.materializationRegistryWeight * 2.0f,
        0.0f,
        255.0f);
}

} // namespace mousefx::windows
