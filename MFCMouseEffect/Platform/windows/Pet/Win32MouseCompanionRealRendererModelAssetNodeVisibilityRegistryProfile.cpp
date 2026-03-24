#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeVisibilityProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeBindingProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

float ResolveVisibilityRegistryWeight(
    const Win32MouseCompanionRealRendererModelAssetNodeVisibilityProfile&
        nodeVisibilityProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile) {
    const float bindingCoverage =
        static_cast<float>(nodeBindingProfile.boundEntryCount) /
        static_cast<float>(std::max<uint32_t>(1u, nodeBindingProfile.entryCount));
    return std::clamp(
        nodeVisibilityProfile.visibilityWeight * 0.77f + bindingCoverage * 0.23f,
        0.0f,
        1.0f);
}

std::string ResolveVisibilityRegistryState(
    const Win32MouseCompanionRealRendererModelAssetNodeVisibilityProfile&
        nodeVisibilityProfile,
    uint32_t resolvedEntryCount,
    uint32_t entryCount,
    const std::string& adapterMode) {
    if (nodeVisibilityProfile.visibilityState == "preview_only" || resolvedEntryCount == 0u) {
        return "preview_only";
    }
    if (resolvedEntryCount >= entryCount) {
        if (adapterMode == "pose_bound") {
            return "model_asset_node_visibility_registry_bound";
        }
        if (adapterMode == "pose_unbound") {
            return "model_asset_node_visibility_registry_pose_ready";
        }
        return "model_asset_node_visibility_registry_ready";
    }
    return "model_asset_node_visibility_registry_partial";
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
    return "body:" + std::string(nodeBindingProfile.bodyEntry.bound ? "node_visibility_registry" : "stub") +
           "|head:" + std::string(nodeBindingProfile.headEntry.bound ? "node_visibility_registry" : "stub") +
           "|appendage:" + std::string(nodeBindingProfile.appendageEntry.bound ? "node_visibility_registry" : "stub") +
           "|overlay:" + std::string(nodeBindingProfile.overlayEntry.bound ? "node_visibility_registry" : "stub") +
           "|grounding:" + std::string(nodeBindingProfile.groundingEntry.bound ? "node_visibility_registry" : "stub") +
           "|adapter:" + (adapterMode.empty() ? "runtime_only" : adapterMode);
}

std::string BuildValueBrief(
    float visibilityRegistryWeight,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    const float bodyWeight = visibilityRegistryWeight * nodeBindingProfile.bodyEntry.bindWeight;
    const float headWeight = visibilityRegistryWeight * nodeBindingProfile.headEntry.bindWeight;
    const float appendageWeight =
        visibilityRegistryWeight * nodeBindingProfile.appendageEntry.bindWeight;
    const float overlayWeight =
        visibilityRegistryWeight * nodeBindingProfile.overlayEntry.bindWeight;
    const float groundingWeight =
        visibilityRegistryWeight * nodeBindingProfile.groundingEntry.bindWeight;
    const float adapterWeight =
        adapterMode == "pose_bound" ? visibilityRegistryWeight :
        (adapterMode == "pose_unbound" ? visibilityRegistryWeight * 0.97f : visibilityRegistryWeight * 0.92f);
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

Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeVisibilityProfile&
        nodeVisibilityProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile profile{};
    profile.entryCount = 5u;
    profile.resolvedEntryCount = nodeBindingProfile.boundEntryCount;
    profile.visibilityRegistryWeight =
        ResolveVisibilityRegistryWeight(nodeVisibilityProfile, nodeBindingProfile);
    profile.visibilityRegistryState = ResolveVisibilityRegistryState(
        nodeVisibilityProfile,
        profile.resolvedEntryCount,
        profile.entryCount,
        adapterMode);
    profile.brief =
        BuildBrief(profile.visibilityRegistryState, profile.entryCount, profile.resolvedEntryCount);
    profile.registryBrief = BuildRegistryBrief(nodeBindingProfile, adapterMode);
    profile.valueBrief =
        BuildValueBrief(profile.visibilityRegistryWeight, nodeBindingProfile, adapterMode);
    return profile;
}

Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    return BuildWin32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile(
        runtime.modelAssetNodeVisibilityProfile,
        runtime.modelNodeBindingProfile,
        runtime.sceneRuntimeAdapterMode);
}

void ApplyWin32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile& profile,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.bodyStrokeWidth *= 1.0f + profile.visibilityRegistryWeight * 0.010f;
    scene.headStrokeWidth *= 1.0f + profile.visibilityRegistryWeight * 0.011f;
    scene.shadowAlphaScale *= 1.0f + profile.visibilityRegistryWeight * 0.012f;
    scene.poseBadgeAlpha = std::clamp(
        scene.poseBadgeAlpha + profile.visibilityRegistryWeight * 2.0f,
        0.0f,
        255.0f);
}

} // namespace mousefx::windows
