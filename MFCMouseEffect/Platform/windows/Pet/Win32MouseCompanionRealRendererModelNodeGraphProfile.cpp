#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererModelNodeGraphProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <cstdio>

namespace mousefx::windows {
namespace {

std::string ResolveGraphState(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    if (runtime.modelAssetNodeLiftProfile.liftState == "model_asset_node_lift_bound") {
        return "channel_bound_preview";
    }
    if (runtime.modelAssetNodeLiftProfile.liftState == "model_asset_node_lift_pose_ready") {
        return "channel_stub_ready";
    }
    if (runtime.modelAssetNodeLiftProfile.liftState == "model_asset_node_lift_ready" ||
        runtime.modelAssetNodeLiftProfile.liftState == "model_asset_node_lift_partial") {
        return "scaffold_ready";
    }
    return "preview_only";
}

uint32_t CountBoundNodes(
    const Win32MouseCompanionRealRendererModelNodeGraphProfile& profile) {
    uint32_t count = 0;
    if (profile.bodyNode.influence > 0.0f) { ++count; }
    if (profile.headNode.influence > 0.0f) { ++count; }
    if (profile.appendageNode.influence > 0.0f) { ++count; }
    if (profile.overlayNode.influence > 0.0f) { ++count; }
    if (profile.groundingNode.influence > 0.0f) { ++count; }
    return count;
}

std::string BuildGraphBrief(
    const std::string& graphState,
    uint32_t nodeCount,
    uint32_t boundNodeCount) {
    char buffer[96];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s/%u/%u",
        graphState.empty() ? "preview_only" : graphState.c_str(),
        nodeCount,
        boundNodeCount);
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererModelNodeGraphProfile
BuildWin32MouseCompanionRealRendererModelNodeGraphProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererModelNodeGraphProfile profile{};
    profile.graphState = ResolveGraphState(runtime);
    profile.nodeCount = 5;

    const auto& adapter = runtime.modelNodeAdapterProfile;
    const float liftWeight = runtime.modelAssetNodeLiftProfile.liftWeight;

    profile.bodyNode.influence = adapter.bodyChannel.influence * liftWeight;
    profile.bodyNode.localOffsetX = adapter.bodyChannel.offsetX;
    profile.bodyNode.localOffsetY = adapter.bodyChannel.offsetY;
    profile.bodyNode.worldOffsetX = adapter.bodyChannel.offsetX * profile.bodyNode.influence;
    profile.bodyNode.worldOffsetY = adapter.bodyChannel.offsetY * profile.bodyNode.influence;

    profile.headNode.influence = adapter.faceChannel.influence * liftWeight;
    profile.headNode.localOffsetX = adapter.faceChannel.offsetX;
    profile.headNode.localOffsetY = adapter.faceChannel.offsetY;
    profile.headNode.worldOffsetX =
        profile.bodyNode.worldOffsetX + adapter.faceChannel.offsetX * profile.headNode.influence;
    profile.headNode.worldOffsetY =
        profile.bodyNode.worldOffsetY + adapter.faceChannel.offsetY * profile.headNode.influence;

    profile.appendageNode.influence = adapter.appendageChannel.influence * liftWeight;
    profile.appendageNode.localOffsetX = adapter.appendageChannel.offsetX;
    profile.appendageNode.localOffsetY = adapter.appendageChannel.offsetY;
    profile.appendageNode.worldOffsetX =
        profile.bodyNode.worldOffsetX + adapter.appendageChannel.offsetX * profile.appendageNode.influence;
    profile.appendageNode.worldOffsetY =
        profile.bodyNode.worldOffsetY + adapter.appendageChannel.offsetY * profile.appendageNode.influence;

    profile.overlayNode.influence = adapter.overlayChannel.influence * liftWeight;
    profile.overlayNode.localOffsetX = adapter.overlayChannel.offsetX;
    profile.overlayNode.localOffsetY = adapter.overlayChannel.offsetY;
    profile.overlayNode.worldOffsetX =
        profile.bodyNode.worldOffsetX + adapter.overlayChannel.offsetX * profile.overlayNode.influence;
    profile.overlayNode.worldOffsetY =
        profile.bodyNode.worldOffsetY + adapter.overlayChannel.offsetY * profile.overlayNode.influence;

    profile.groundingNode.influence = adapter.groundingChannel.influence * liftWeight;
    profile.groundingNode.localOffsetX = adapter.groundingChannel.offsetX;
    profile.groundingNode.localOffsetY = adapter.groundingChannel.offsetY;
    profile.groundingNode.worldOffsetX =
        profile.bodyNode.worldOffsetX + adapter.groundingChannel.offsetX * profile.groundingNode.influence;
    profile.groundingNode.worldOffsetY =
        profile.bodyNode.worldOffsetY + adapter.groundingChannel.offsetY * profile.groundingNode.influence;

    profile.boundNodeCount = CountBoundNodes(profile);
    profile.brief = BuildGraphBrief(
        profile.graphState,
        profile.nodeCount,
        profile.boundNodeCount);
    return profile;
}

} // namespace mousefx::windows
