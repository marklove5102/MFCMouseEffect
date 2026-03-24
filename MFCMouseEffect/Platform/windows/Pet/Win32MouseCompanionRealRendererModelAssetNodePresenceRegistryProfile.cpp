#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelAssetNodePresenceProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeBindingProfile.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererScene.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

float ResolvePresenceRegistryWeight(
    const Win32MouseCompanionRealRendererModelAssetNodePresenceProfile& nodePresenceProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile) {
    const float bindingCoverage =
        static_cast<float>(nodeBindingProfile.boundEntryCount) /
        static_cast<float>(std::max<uint32_t>(1u, nodeBindingProfile.entryCount));
    return std::clamp(
        nodePresenceProfile.presenceWeight * 0.78f + bindingCoverage * 0.22f,
        0.0f,
        1.0f);
}

std::string ResolvePresenceRegistryState(
    const Win32MouseCompanionRealRendererModelAssetNodePresenceProfile& nodePresenceProfile,
    uint32_t resolvedEntryCount,
    uint32_t entryCount,
    const std::string& adapterMode) {
    if (nodePresenceProfile.presenceState == "preview_only" || resolvedEntryCount == 0u) {
        return "preview_only";
    }
    if (resolvedEntryCount >= entryCount) {
        if (adapterMode == "pose_bound") {
            return "model_asset_node_presence_registry_bound";
        }
        if (adapterMode == "pose_unbound") {
            return "model_asset_node_presence_registry_pose_ready";
        }
        return "model_asset_node_presence_registry_ready";
    }
    return "model_asset_node_presence_registry_partial";
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
    return "body:" + std::string(nodeBindingProfile.bodyEntry.bound ? "node_presence_registry" : "stub") +
           "|head:" + std::string(nodeBindingProfile.headEntry.bound ? "node_presence_registry" : "stub") +
           "|appendage:" +
               std::string(nodeBindingProfile.appendageEntry.bound ? "node_presence_registry" : "stub") +
           "|overlay:" +
               std::string(nodeBindingProfile.overlayEntry.bound ? "node_presence_registry" : "stub") +
           "|grounding:" +
               std::string(nodeBindingProfile.groundingEntry.bound ? "node_presence_registry" : "stub") +
           "|adapter:" + (adapterMode.empty() ? "runtime_only" : adapterMode);
}

std::string BuildValueBrief(
    float presenceRegistryWeight,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    const float bodyWeight = presenceRegistryWeight * nodeBindingProfile.bodyEntry.bindWeight;
    const float headWeight = presenceRegistryWeight * nodeBindingProfile.headEntry.bindWeight;
    const float appendageWeight = presenceRegistryWeight * nodeBindingProfile.appendageEntry.bindWeight;
    const float overlayWeight = presenceRegistryWeight * nodeBindingProfile.overlayEntry.bindWeight;
    const float groundingWeight = presenceRegistryWeight * nodeBindingProfile.groundingEntry.bindWeight;
    const float adapterWeight =
        adapterMode == "pose_bound" ? presenceRegistryWeight :
        (adapterMode == "pose_unbound" ? presenceRegistryWeight * 0.97f : presenceRegistryWeight * 0.92f);
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

Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodePresenceProfile& nodePresenceProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode) {
    Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile profile{};
    profile.entryCount = 5u;
    profile.resolvedEntryCount = nodeBindingProfile.boundEntryCount;
    profile.presenceRegistryWeight =
        ResolvePresenceRegistryWeight(nodePresenceProfile, nodeBindingProfile);
    profile.presenceRegistryState = ResolvePresenceRegistryState(
        nodePresenceProfile,
        profile.resolvedEntryCount,
        profile.entryCount,
        adapterMode);
    profile.brief = BuildBrief(
        profile.presenceRegistryState,
        profile.entryCount,
        profile.resolvedEntryCount);
    profile.registryBrief = BuildRegistryBrief(nodeBindingProfile, adapterMode);
    profile.valueBrief =
        BuildValueBrief(profile.presenceRegistryWeight, nodeBindingProfile, adapterMode);
    return profile;
}

Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    return BuildWin32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile(
        runtime.modelAssetNodePresenceProfile,
        runtime.modelNodeBindingProfile,
        runtime.sceneRuntimeAdapterMode);
}

void ApplyWin32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile& profile,
    Win32MouseCompanionRealRendererScene& scene) {
    scene.bodyStrokeWidth *= 1.0f + profile.presenceRegistryWeight * 0.009f;
    scene.headStrokeWidth *= 1.0f + profile.presenceRegistryWeight * 0.010f;
    scene.poseBadgeAlpha = std::clamp(
        scene.poseBadgeAlpha + profile.presenceRegistryWeight * 2.0f,
        0.0f,
        255.0f);
}

} // namespace mousefx::windows
