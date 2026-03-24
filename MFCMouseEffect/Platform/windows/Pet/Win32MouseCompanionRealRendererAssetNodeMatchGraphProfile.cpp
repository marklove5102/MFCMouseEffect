#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeMatchGraphProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cctype>
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

std::string NormalizeNodeToken(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch) != 0) {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return normalized;
}

bool ContainsToken(const std::string& normalizedName, const char* token) {
    if (token == nullptr || *token == '\0') {
        return false;
    }
    return normalizedName.find(token) != std::string::npos;
}

const Win32MouseCompanionRealRendererGlbNodeEntry* ResolveMatchedNode(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    const Win32MouseCompanionRealRendererAssetNodeMatchQueryEntry& queryEntry) {
    if (runtime.assets == nullptr || !runtime.assets->modelNodeTreeLoaded) {
        return nullptr;
    }

    const auto& nodes = runtime.assets->modelNodeTree.nodes;
    auto scoreNode = [&](const Win32MouseCompanionRealRendererGlbNodeEntry& node) {
        const std::string normalizedName = NormalizeNodeToken(node.nodeName);
        int score = 0;

        if (queryEntry.logicalNode == "body") {
            if (ContainsToken(normalizedName, "spine")) { score += 10; }
            if (ContainsToken(normalizedName, "hips")) { score += 8; }
            if (ContainsToken(normalizedName, "rootjoint")) { score += 7; }
            if (ContainsToken(normalizedName, "torso")) { score += 6; }
        } else if (queryEntry.logicalNode == "head") {
            if (ContainsToken(normalizedName, "head")) { score += 12; }
            if (ContainsToken(normalizedName, "neck")) { score += 8; }
        } else if (queryEntry.logicalNode == "appendage") {
            if (ContainsToken(normalizedName, "arm")) { score += 10; }
            if (ContainsToken(normalizedName, "ear")) { score += 8; }
            if (ContainsToken(normalizedName, "leg")) { score += 6; }
            if (ContainsToken(normalizedName, "tail")) { score += 5; }
        } else if (queryEntry.logicalNode == "overlay") {
            if (ContainsToken(normalizedName, "object")) { score += 8; }
            if (ContainsToken(normalizedName, "effect")) { score += 8; }
            if (ContainsToken(normalizedName, "overlay")) { score += 8; }
        } else if (queryEntry.logicalNode == "grounding") {
            if (ContainsToken(normalizedName, "root")) { score += 10; }
            if (ContainsToken(normalizedName, "leg")) { score += 7; }
            if (ContainsToken(normalizedName, "spine")) { score += 4; }
        }

        const std::string aliasToken = NormalizeNodeToken(queryEntry.queryAlias);
        const std::string labelToken = NormalizeNodeToken(queryEntry.queryNodeLabel);
        if (!aliasToken.empty() && normalizedName.find(aliasToken) != std::string::npos) {
            score += 4;
        }
        if (!labelToken.empty() && normalizedName.find(labelToken) != std::string::npos) {
            score += 3;
        }
        if (!queryEntry.queryTokenSeed.empty() &&
            normalizedName.find(NormalizeNodeToken(queryEntry.queryTokenSeed)) != std::string::npos) {
            score += 2;
        }
        return score;
    };

    const Win32MouseCompanionRealRendererGlbNodeEntry* bestNode = nullptr;
    int bestScore = 0;
    for (const auto& node : nodes) {
        const int score = scoreNode(node);
        if (score > bestScore) {
            bestScore = score;
            bestNode = &node;
        }
    }
    return bestScore > 0 ? bestNode : nullptr;
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

    const auto* matchedNode = ResolveMatchedNode(runtime, queryEntry);
    if (matchedNode != nullptr && runtime.assets != nullptr) {
        entry.graphLocator = matchedNode->nodePath;
        entry.graphNodeKey = runtime.assets->modelRootNodeKey + "#" +
            std::to_string(matchedNode->nodeIndex);
        entry.graphNodeLabel = matchedNode->nodeName;
        entry.graphAlias = matchedNode->nodeName;
        entry.graphTokenSeed = matchedNode->nodePath;
        entry.graphConfidence = std::clamp(entry.graphConfidence + 0.2f, 0.0f, 1.0f);
        entry.resolved = true;
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
        profile.bodyEntry.graphNodeLabel.c_str(),
        profile.headEntry.graphNodeLabel.c_str(),
        profile.appendageEntry.graphNodeLabel.c_str(),
        profile.overlayEntry.graphNodeLabel.c_str(),
        profile.groundingEntry.graphNodeLabel.c_str());
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
