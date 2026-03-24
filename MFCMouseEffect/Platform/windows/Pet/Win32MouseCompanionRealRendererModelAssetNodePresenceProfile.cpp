#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodePresenceProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeRegistryProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

float ResolvePresenceWeight(
    const Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile&
        nodeVisibilityRegistryProfile,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile) {
    const float registryCoverage =
        static_cast<float>(nodeRegistryProfile.resolvedEntryCount) /
        static_cast<float>(std::max<uint32_t>(1u, nodeRegistryProfile.entryCount));
    return std::clamp(
        nodeVisibilityRegistryProfile.visibilityRegistryWeight * 0.79f +
            registryCoverage * 0.21f,
        0.0f,
        1.0f);
}

std::string ResolvePresenceState(
    const Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile&
        nodeVisibilityRegistryProfile,
    uint32_t resolvedEntryCount,
    uint32_t entryCount,
    const std::string& adapterMode) {
    if (nodeVisibilityRegistryProfile.visibilityRegistryState == "preview_only" ||
        resolvedEntryCount == 0u) {
        return "preview_only";
    }
    if (resolvedEntryCount >= entryCount) {
        if (adapterMode == "pose_bound") {
            return "model_asset_node_presence_bound";
        }
        if (adapterMode == "pose_unbound") {
            return "model_asset_node_presence_pose_ready";
        }
        return "model_asset_node_presence_ready";
    }
    return "model_asset_node_presence_partial";
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

std::string BuildPresenceBrief(
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    return "body:" + std::string(nodeRegistryProfile.bodyEntry.resolved ? "node_presence" : "stub") +
           "|head:" + std::string(nodeRegistryProfile.headEntry.resolved ? "node_presence" : "stub") +
           "|appendage:" +
               std::string(nodeRegistryProfile.appendageEntry.resolved ? "node_presence" : "stub") +
           "|overlay:" +
               std::string(nodeRegistryProfile.overlayEntry.resolved ? "node_presence" : "stub") +
           "|grounding:" +
               std::string(nodeRegistryProfile.groundingEntry.resolved ? "node_presence" : "stub") +
           "|adapter:" + (adapterMode.empty() ? "runtime_only" : adapterMode);
}

std::string BuildValueBrief(
    float presenceWeight,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    const float bodyWeight = presenceWeight * nodeRegistryProfile.bodyEntry.registryWeight;
    const float headWeight = presenceWeight * nodeRegistryProfile.headEntry.registryWeight;
    const float appendageWeight = presenceWeight * nodeRegistryProfile.appendageEntry.registryWeight;
    const float overlayWeight = presenceWeight * nodeRegistryProfile.overlayEntry.registryWeight;
    const float groundingWeight = presenceWeight * nodeRegistryProfile.groundingEntry.registryWeight;
    const float adapterWeight =
        adapterMode == "pose_bound" ? presenceWeight :
        (adapterMode == "pose_unbound" ? presenceWeight * 0.97f : presenceWeight * 0.92f);
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

Win32MouseCompanionRealRendererModelAssetNodePresenceProfile
BuildWin32MouseCompanionRealRendererModelAssetNodePresenceProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeVisibilityRegistryProfile&
        nodeVisibilityRegistryProfile,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode) {
    Win32MouseCompanionRealRendererModelAssetNodePresenceProfile profile{};
    profile.entryCount = 5u;
    profile.resolvedEntryCount = nodeRegistryProfile.resolvedEntryCount;
    profile.presenceWeight =
        ResolvePresenceWeight(nodeVisibilityRegistryProfile, nodeRegistryProfile);
    profile.presenceState = ResolvePresenceState(
        nodeVisibilityRegistryProfile,
        profile.resolvedEntryCount,
        profile.entryCount,
        adapterMode);
    profile.brief =
        BuildBrief(profile.presenceState, profile.entryCount, profile.resolvedEntryCount);
    profile.presenceBrief = BuildPresenceBrief(nodeRegistryProfile, adapterMode);
    profile.valueBrief = BuildValueBrief(profile.presenceWeight, nodeRegistryProfile, adapterMode);
    return profile;
}

Win32MouseCompanionRealRendererModelAssetNodePresenceProfile
BuildWin32MouseCompanionRealRendererModelAssetNodePresenceProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    return BuildWin32MouseCompanionRealRendererModelAssetNodePresenceProfile(
        runtime.modelAssetNodeVisibilityRegistryProfile,
        runtime.modelNodeRegistryProfile,
        runtime.sceneRuntimeAdapterMode);
}

void ApplyWin32MouseCompanionRealRendererModelAssetNodePresenceProfile(
    const Win32MouseCompanionRealRendererModelAssetNodePresenceProfile& profile,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.glowAlpha = std::clamp(scene.glowAlpha + profile.presenceWeight * 6.0f, 0.0f, 255.0f);
    scene.pedestalAlphaScale *= 1.0f + profile.presenceWeight * 0.010f;
    scene.accessoryAlphaScale *= 1.0f + profile.presenceWeight * 0.008f;
}

} // namespace mousefx::windows
