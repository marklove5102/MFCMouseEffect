#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeDriverRegistryProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeRegistryProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

float ResolveConsumerWeight(
    const Win32MouseCompanionRealRendererModelAssetNodeDriverRegistryProfile& nodeDriverRegistryProfile,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile) {
    const float registryCoverage =
        static_cast<float>(nodeRegistryProfile.resolvedEntryCount) /
        static_cast<float>(std::max<uint32_t>(1u, nodeRegistryProfile.entryCount));
    return std::clamp(
        nodeDriverRegistryProfile.driverRegistryWeight * 0.74f + registryCoverage * 0.26f,
        0.0f,
        1.0f);
}

std::string ResolveConsumerState(
    const Win32MouseCompanionRealRendererModelAssetNodeDriverRegistryProfile& nodeDriverRegistryProfile,
    uint32_t resolvedEntryCount,
    uint32_t entryCount,
    const std::string& adapterMode) {
    if (nodeDriverRegistryProfile.driverRegistryState == "preview_only" ||
        resolvedEntryCount == 0u) {
        return "preview_only";
    }
    if (resolvedEntryCount >= entryCount) {
        if (adapterMode == "pose_bound") {
            return "model_asset_node_consumer_bound";
        }
        if (adapterMode == "pose_unbound") {
            return "model_asset_node_consumer_pose_ready";
        }
        return "model_asset_node_consumer_ready";
    }
    return "model_asset_node_consumer_partial";
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

std::string BuildConsumerBrief(
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    return "body:" + std::string(nodeRegistryProfile.bodyEntry.resolved ? "node_consumer" : "stub") +
           "|head:" + std::string(nodeRegistryProfile.headEntry.resolved ? "node_consumer" : "stub") +
           "|appendage:" + std::string(nodeRegistryProfile.appendageEntry.resolved ? "node_consumer" : "stub") +
           "|overlay:" + std::string(nodeRegistryProfile.overlayEntry.resolved ? "node_consumer" : "stub") +
           "|grounding:" + std::string(nodeRegistryProfile.groundingEntry.resolved ? "node_consumer" : "stub") +
           "|adapter:" + (adapterMode.empty() ? "runtime_only" : adapterMode);
}

std::string BuildValueBrief(
    float consumerWeight,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    const float bodyWeight = consumerWeight * nodeRegistryProfile.bodyEntry.registryWeight;
    const float headWeight = consumerWeight * nodeRegistryProfile.headEntry.registryWeight;
    const float appendageWeight = consumerWeight * nodeRegistryProfile.appendageEntry.registryWeight;
    const float overlayWeight = consumerWeight * nodeRegistryProfile.overlayEntry.registryWeight;
    const float groundingWeight = consumerWeight * nodeRegistryProfile.groundingEntry.registryWeight;
    const float adapterWeight =
        adapterMode == "pose_bound" ? consumerWeight :
        (adapterMode == "pose_unbound" ? consumerWeight * 0.96f : consumerWeight * 0.82f);
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

Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeDriverRegistryProfile& nodeDriverRegistryProfile,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile profile{};
    profile.entryCount = 5u;
    profile.resolvedEntryCount = nodeRegistryProfile.resolvedEntryCount;
    profile.consumerWeight =
        ResolveConsumerWeight(nodeDriverRegistryProfile, nodeRegistryProfile);
    profile.consumerState = ResolveConsumerState(
        nodeDriverRegistryProfile,
        profile.resolvedEntryCount,
        profile.entryCount,
        adapterMode);
    profile.brief =
        BuildBrief(profile.consumerState, profile.entryCount, profile.resolvedEntryCount);
    profile.consumerBrief = BuildConsumerBrief(nodeRegistryProfile, adapterMode);
    profile.valueBrief = BuildValueBrief(profile.consumerWeight, nodeRegistryProfile, adapterMode);
    return profile;
}

Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    return BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerProfile(
        runtime.modelAssetNodeDriverRegistryProfile,
        runtime.modelNodeRegistryProfile,
        runtime.sceneRuntimeAdapterMode);
}

void ApplyWin32MouseCompanionRealRendererModelAssetNodeConsumerProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile& profile,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.bodyAnchorScale *= 1.0f + profile.consumerWeight * 0.008f;
    scene.headAnchorScale *= 1.0f + profile.consumerWeight * 0.009f;
    scene.accessoryAlphaScale *= 1.0f + profile.consumerWeight * 0.010f;
    scene.eyeHighlightAlpha = std::clamp(
        scene.eyeHighlightAlpha + profile.consumerWeight * 2.0f,
        0.0f,
        255.0f);
}

} // namespace mousefx::windows
