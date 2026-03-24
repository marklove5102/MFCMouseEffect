#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererGlbNodeMatcher.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererNodeMatchNaming.h"
#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

std::string ResolveGraphState(const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    if (runtime.realModelNodeTreeLoaded && runtime.realModelNodeCount > 0) {
        return "match_graph_glb_ready";
    }
    const std::string& queryState = runtime.assetNodeMatchQueryProfile.queryState;
    if (queryState == "match_query_ready") {
        return "match_graph_ready";
    }
    if (queryState == "match_query_stub_ready") {
        return "match_graph_stub_ready";
    }
    if (queryState == "match_query_scaffold") {
        return "match_graph_scaffold";
    }
    return "preview_only";
}

std::string ResolveGraphAlias(
    const Win32MouseCompanionRealRendererAssetNodeMatchQueryEntry& queryEntry) {
    if (!queryEntry.queryAlias.empty()) {
        return queryEntry.queryAlias;
    }
    return queryEntry.logicalNode + "_graph";
}

Win32MouseCompanionRealRendererAssetNodeMatchGraphEntry BuildGraphEntry(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    const Win32MouseCompanionRealRendererAssetNodeMatchQueryEntry& queryEntry) {
    Win32MouseCompanionRealRendererAssetNodeMatchGraphEntry entry{};
    entry.logicalNode = queryEntry.logicalNode;
    entry.graphLocator = queryEntry.queryLocator;
    entry.graphNodeKey = queryEntry.queryNodeKey;
    entry.graphNodeLabel = queryEntry.queryNodeLabel;
    entry.graphAlias = ResolveGraphAlias(queryEntry);
    entry.graphTokenSeed = queryEntry.queryTokenSeed;
    entry.graphConfidence = std::clamp(
        queryEntry.queryConfidence +
            (!entry.graphAlias.empty() ? 0.02f : 0.0f) +
            (!entry.graphTokenSeed.empty() ? 0.02f : 0.0f),
        0.0f,
        1.0f);
    entry.resolved = queryEntry.resolved;

    if (runtime.assets != nullptr && runtime.assets->modelNodeTreeLoaded) {
        const auto match = ResolveWin32MouseCompanionRealRendererGlbNodeMatch(
            runtime.assets->modelNodeTree,
            queryEntry.logicalNode,
            queryEntry.queryLocator,
            queryEntry.queryNodeLabel,
            queryEntry.queryAlias,
            queryEntry.queryNodeLabel,
            queryEntry.queryTokenSeed);
        if (match.matched) {
            entry.graphLocator = match.nodePath;
            entry.graphNodeKey = runtime.assets->modelRootNodeKey + "#" +
                std::to_string(match.nodeIndex);
            entry.graphNodeLabel = match.nodeName;
            entry.graphAlias = match.nodeName;
            entry.graphTokenSeed = match.nodePath;
            entry.matchBasis = match.matchBasis;
            entry.matchedNodeIndex = match.nodeIndex;
            entry.graphConfidence = std::max(entry.graphConfidence, match.confidence);
            entry.resolved = true;
        }
    }

    if (entry.matchBasis.empty()) {
        entry.matchBasis =
            NormalizeWin32MouseCompanionNodeMatchToken(entry.graphAlias).empty()
                ? "query"
                : "alias";
    }
    return entry;
}

uint32_t CountResolvedEntries(
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile& profile) {
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

std::string BuildLocatorBrief(
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile& profile) {
    char buffer[1024];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.graphLocator.c_str(),
        profile.headEntry.graphLocator.c_str(),
        profile.appendageEntry.graphLocator.c_str(),
        profile.overlayEntry.graphLocator.c_str(),
        profile.groundingEntry.graphLocator.c_str());
    return std::string(buffer);
}

std::string BuildValueBrief(
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile& profile) {
    char buffer[1024];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        (profile.bodyEntry.graphNodeLabel + "@" + profile.bodyEntry.matchBasis).c_str(),
        (profile.headEntry.graphNodeLabel + "@" + profile.headEntry.matchBasis).c_str(),
        (profile.appendageEntry.graphNodeLabel + "@" + profile.appendageEntry.matchBasis).c_str(),
        (profile.overlayEntry.graphNodeLabel + "@" + profile.overlayEntry.matchBasis).c_str(),
        (profile.groundingEntry.graphNodeLabel + "@" + profile.groundingEntry.matchBasis).c_str());
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile
BuildWin32MouseCompanionRealRendererAssetNodeMatchGraphProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile profile{};
    profile.graphState = ResolveGraphState(runtime);
    profile.entryCount = 5;

    const auto& queryProfile = runtime.assetNodeMatchQueryProfile;
    profile.bodyEntry = BuildGraphEntry(runtime, queryProfile.bodyEntry);
    profile.headEntry = BuildGraphEntry(runtime, queryProfile.headEntry);
    profile.appendageEntry = BuildGraphEntry(runtime, queryProfile.appendageEntry);
    profile.overlayEntry = BuildGraphEntry(runtime, queryProfile.overlayEntry);
    profile.groundingEntry = BuildGraphEntry(runtime, queryProfile.groundingEntry);

    profile.resolvedEntryCount = CountResolvedEntries(profile);
    profile.brief = BuildBrief(profile.graphState, profile.entryCount, profile.resolvedEntryCount);
    profile.locatorBrief = BuildLocatorBrief(profile);
    profile.valueBrief = BuildValueBrief(profile);
    return profile;
}

} // namespace mousefx::windows
