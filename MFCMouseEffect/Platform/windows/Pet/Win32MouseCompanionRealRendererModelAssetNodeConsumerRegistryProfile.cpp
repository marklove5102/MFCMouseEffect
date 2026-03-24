#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeBindingProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

float ResolveConsumerRegistryWeight(
    const Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile& nodeConsumerProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile) {
    const float bindingCoverage =
        static_cast<float>(nodeBindingProfile.boundEntryCount) /
        static_cast<float>(std::max<uint32_t>(1u, nodeBindingProfile.entryCount));
    return std::clamp(
        nodeConsumerProfile.consumerWeight * 0.72f + bindingCoverage * 0.28f,
        0.0f,
        1.0f);
}

std::string ResolveConsumerRegistryState(
    const Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile& nodeConsumerProfile,
    uint32_t resolvedEntryCount,
    uint32_t entryCount,
    const std::string& adapterMode) {
    if (nodeConsumerProfile.consumerState == "preview_only" || resolvedEntryCount == 0u) {
        return "preview_only";
    }
    if (resolvedEntryCount >= entryCount) {
        if (adapterMode == "pose_bound") {
            return "model_asset_node_consumer_registry_bound";
        }
        if (adapterMode == "pose_unbound") {
            return "model_asset_node_consumer_registry_pose_ready";
        }
        return "model_asset_node_consumer_registry_ready";
    }
    return "model_asset_node_consumer_registry_partial";
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
    return "body:" + std::string(nodeBindingProfile.bodyEntry.bound ? "node_consumer_registry" : "stub") +
           "|head:" + std::string(nodeBindingProfile.headEntry.bound ? "node_consumer_registry" : "stub") +
           "|appendage:" + std::string(nodeBindingProfile.appendageEntry.bound ? "node_consumer_registry" : "stub") +
           "|overlay:" + std::string(nodeBindingProfile.overlayEntry.bound ? "node_consumer_registry" : "stub") +
           "|grounding:" + std::string(nodeBindingProfile.groundingEntry.bound ? "node_consumer_registry" : "stub") +
           "|adapter:" + (adapterMode.empty() ? "runtime_only" : adapterMode);
}

std::string BuildValueBrief(
    float consumerRegistryWeight,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    const float bodyWeight = consumerRegistryWeight * nodeBindingProfile.bodyEntry.bindWeight;
    const float headWeight = consumerRegistryWeight * nodeBindingProfile.headEntry.bindWeight;
    const float appendageWeight = consumerRegistryWeight * nodeBindingProfile.appendageEntry.bindWeight;
    const float overlayWeight = consumerRegistryWeight * nodeBindingProfile.overlayEntry.bindWeight;
    const float groundingWeight = consumerRegistryWeight * nodeBindingProfile.groundingEntry.bindWeight;
    const float adapterWeight =
        adapterMode == "pose_bound" ? consumerRegistryWeight :
        (adapterMode == "pose_unbound" ? consumerRegistryWeight * 0.96f : consumerRegistryWeight * 0.84f);
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

Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile& nodeConsumerProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile profile{};
    profile.entryCount = 5u;
    profile.resolvedEntryCount = nodeBindingProfile.boundEntryCount;
    profile.consumerRegistryWeight =
        ResolveConsumerRegistryWeight(nodeConsumerProfile, nodeBindingProfile);
    profile.consumerRegistryState = ResolveConsumerRegistryState(
        nodeConsumerProfile,
        profile.resolvedEntryCount,
        profile.entryCount,
        adapterMode);
    profile.brief =
        BuildBrief(profile.consumerRegistryState, profile.entryCount, profile.resolvedEntryCount);
    profile.registryBrief = BuildRegistryBrief(nodeBindingProfile, adapterMode);
    profile.valueBrief =
        BuildValueBrief(profile.consumerRegistryWeight, nodeBindingProfile, adapterMode);
    return profile;
}

Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    return BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile(
        runtime.modelAssetNodeConsumerProfile,
        runtime.modelNodeBindingProfile,
        runtime.sceneRuntimeAdapterMode);
}

void ApplyWin32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile& profile,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.bodyStrokeWidth *= 1.0f + profile.consumerRegistryWeight * 0.008f;
    scene.headStrokeWidth *= 1.0f + profile.consumerRegistryWeight * 0.009f;
    scene.shadowAlphaScale *= 1.0f + profile.consumerRegistryWeight * 0.008f;
    scene.poseBadgeAlpha = std::clamp(
        scene.poseBadgeAlpha + profile.consumerRegistryWeight * 2.0f,
        0.0f,
        255.0f);
}

} // namespace mousefx::windows
