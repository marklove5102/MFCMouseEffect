#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeMatchResolveProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

std::string ResolveResolveState(const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    const std::string& planState = runtime.assetNodeMatchPlanProfile.planState;
    if (planState == "match_plan_ready") {
        return "match_resolve_ready";
    }
    if (planState == "match_plan_stub_ready") {
        return "match_resolve_stub_ready";
    }
    if (planState == "match_plan_scaffold") {
        return "match_resolve_scaffold";
    }
    return "preview_only";
}

std::string ResolveRouteState(
    const Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry& planEntry) {
    if (!planEntry.parserLocator.empty() && planEntry.parserLocator.rfind("parser://", 0) == 0) {
        return "parser_probe";
    }
    if (!planEntry.fallbackAlias.empty()) {
        return "fallback_alias";
    }
    return "preview_stub";
}

std::string ResolveFinalNodeKey(
    const Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry& planEntry) {
    if (!planEntry.probeKey.empty()) {
        return planEntry.probeKey;
    }
    if (!planEntry.planTokenSeed.empty()) {
        return planEntry.logicalNode + "|" + planEntry.planTokenSeed;
    }
    return planEntry.logicalNode + "|resolve";
}

std::string ResolveFinalNodeLabel(
    const Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry& planEntry,
    const std::string& routeState) {
    if (!planEntry.probeLabel.empty()) {
        return planEntry.probeLabel + "@" + routeState;
    }
    return planEntry.logicalNode + "@resolve";
}

Win32MouseCompanionRealRendererAssetNodeMatchResolveEntry BuildResolveEntry(
    const Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry& planEntry) {
    Win32MouseCompanionRealRendererAssetNodeMatchResolveEntry entry{};
    entry.logicalNode = planEntry.logicalNode;
    entry.parserLocator = planEntry.parserLocator;
    entry.probeKey = planEntry.probeKey;
    entry.probeLabel = planEntry.probeLabel;
    entry.routeState = ResolveRouteState(planEntry);
    entry.finalNodeKey = ResolveFinalNodeKey(planEntry);
    entry.finalNodeLabel = ResolveFinalNodeLabel(planEntry, entry.routeState);
    entry.resolveConfidence = std::clamp(
        planEntry.planConfidence +
            (!entry.finalNodeKey.empty() ? 0.04f : 0.0f) +
            (!entry.routeState.empty() && entry.routeState != "preview_stub" ? 0.04f : 0.0f),
        0.0f,
        1.0f);
    entry.resolved = planEntry.resolved;
    return entry;
}

uint32_t CountResolvedEntries(
    const Win32MouseCompanionRealRendererAssetNodeMatchResolveProfile& profile) {
    uint32_t count = 0;
    if (profile.bodyEntry.resolved) { ++count; }
    if (profile.headEntry.resolved) { ++count; }
    if (profile.appendageEntry.resolved) { ++count; }
    if (profile.overlayEntry.resolved) { ++count; }
    if (profile.groundingEntry.resolved) { ++count; }
    return count;
}

std::string BuildBrief(
    const std::string& state,
    uint32_t entryCount,
    uint32_t resolvedEntryCount) {
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

std::string BuildKeyBrief(
    const Win32MouseCompanionRealRendererAssetNodeMatchResolveProfile& profile) {
    char buffer[1024];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.finalNodeKey.c_str(),
        profile.headEntry.finalNodeKey.c_str(),
        profile.appendageEntry.finalNodeKey.c_str(),
        profile.overlayEntry.finalNodeKey.c_str(),
        profile.groundingEntry.finalNodeKey.c_str());
    return std::string(buffer);
}

std::string BuildLabelBrief(
    const Win32MouseCompanionRealRendererAssetNodeMatchResolveProfile& profile) {
    char buffer[1024];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.finalNodeLabel.c_str(),
        profile.headEntry.finalNodeLabel.c_str(),
        profile.appendageEntry.finalNodeLabel.c_str(),
        profile.overlayEntry.finalNodeLabel.c_str(),
        profile.groundingEntry.finalNodeLabel.c_str());
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererAssetNodeMatchResolveProfile
BuildWin32MouseCompanionRealRendererAssetNodeMatchResolveProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererAssetNodeMatchResolveProfile profile{};
    profile.resolveState = ResolveResolveState(runtime);
    profile.entryCount = 5;

    const auto& planProfile = runtime.assetNodeMatchPlanProfile;
    profile.bodyEntry = BuildResolveEntry(planProfile.bodyEntry);
    profile.headEntry = BuildResolveEntry(planProfile.headEntry);
    profile.appendageEntry = BuildResolveEntry(planProfile.appendageEntry);
    profile.overlayEntry = BuildResolveEntry(planProfile.overlayEntry);
    profile.groundingEntry = BuildResolveEntry(planProfile.groundingEntry);

    profile.resolvedEntryCount = CountResolvedEntries(profile);
    profile.brief =
        BuildBrief(profile.resolveState, profile.entryCount, profile.resolvedEntryCount);
    profile.keyBrief = BuildKeyBrief(profile);
    profile.labelBrief = BuildLabelBrief(profile);
    return profile;
}

} // namespace mousefx::windows
