#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeMatchCatalogProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <cstdio>

namespace mousefx::windows {
namespace {

std::string ResolveCatalogState(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    const std::string& resolverState = runtime.assetNodeTargetResolverProfile.resolverState;
    if (resolverState == "target_resolver_ready") {
        return "match_catalog_ready";
    }
    if (resolverState == "target_resolver_stub_ready") {
        return "match_catalog_stub_ready";
    }
    if (resolverState == "target_resolver_scaffold") {
        return "match_catalog_scaffold";
    }
    return "preview_only";
}

std::string ResolveMatchSource(
    const Win32MouseCompanionRealRendererAssetNodeTargetResolverEntry& entry) {
    if (!entry.selectorKey.empty() && !entry.candidateNodeName.empty() &&
        entry.candidateNodeName != "unknown") {
        return "selector_candidate";
    }
    if (!entry.selectorKey.empty()) {
        return "selector_only";
    }
    if (!entry.candidateNodeName.empty() && entry.candidateNodeName != "unknown") {
        return "candidate_only";
    }
    return "preview_stub";
}

std::string ResolveCanonicalNodeKey(
    const Win32MouseCompanionRealRendererAssetNodeTargetResolverEntry& entry) {
    if (!entry.resolvedNodeKey.empty()) {
        return entry.resolvedNodeKey;
    }
    if (!entry.selectorKey.empty()) {
        return entry.selectorKey;
    }
    if (!entry.candidateNodeName.empty()) {
        return entry.logicalNode + "|candidate:" + entry.candidateNodeName;
    }
    return entry.logicalNode + "|preview";
}

std::string ResolveCanonicalNodeLabel(
    const Win32MouseCompanionRealRendererAssetNodeTargetResolverEntry& entry,
    const std::string& matchSource) {
    if (!entry.resolvedNodeLabel.empty()) {
        return entry.resolvedNodeLabel + "@" + matchSource;
    }
    if (!entry.candidateNodeName.empty()) {
        return entry.logicalNode + "@" + entry.candidateNodeName + "@" + matchSource;
    }
    return entry.logicalNode + "@preview";
}

Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry BuildCatalogEntry(
    const Win32MouseCompanionRealRendererAssetNodeTargetResolverEntry& entry) {
    Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry catalogEntry{};
    catalogEntry.logicalNode = entry.logicalNode;
    catalogEntry.selectorKey = entry.selectorKey;
    catalogEntry.candidateNodeName = entry.candidateNodeName;
    catalogEntry.resolvedNodeKey = entry.resolvedNodeKey;
    catalogEntry.resolvedNodeLabel = entry.resolvedNodeLabel;
    catalogEntry.matchSource = ResolveMatchSource(entry);
    catalogEntry.canonicalNodeKey = ResolveCanonicalNodeKey(entry);
    catalogEntry.canonicalNodeLabel =
        ResolveCanonicalNodeLabel(entry, catalogEntry.matchSource);
    catalogEntry.matchConfidence = entry.matchConfidence;
    catalogEntry.resolved = entry.resolved;
    return catalogEntry;
}

uint32_t CountResolvedEntries(
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogProfile& profile) {
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
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogProfile& profile) {
    char buffer[768];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.canonicalNodeKey.c_str(),
        profile.headEntry.canonicalNodeKey.c_str(),
        profile.appendageEntry.canonicalNodeKey.c_str(),
        profile.overlayEntry.canonicalNodeKey.c_str(),
        profile.groundingEntry.canonicalNodeKey.c_str());
    return std::string(buffer);
}

std::string BuildLabelBrief(
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogProfile& profile) {
    char buffer[768];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.canonicalNodeLabel.c_str(),
        profile.headEntry.canonicalNodeLabel.c_str(),
        profile.appendageEntry.canonicalNodeLabel.c_str(),
        profile.overlayEntry.canonicalNodeLabel.c_str(),
        profile.groundingEntry.canonicalNodeLabel.c_str());
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererAssetNodeMatchCatalogProfile
BuildWin32MouseCompanionRealRendererAssetNodeMatchCatalogProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererAssetNodeMatchCatalogProfile profile{};
    profile.catalogState = ResolveCatalogState(runtime);
    profile.entryCount = 5;

    const auto& resolverProfile = runtime.assetNodeTargetResolverProfile;
    profile.bodyEntry = BuildCatalogEntry(resolverProfile.bodyEntry);
    profile.headEntry = BuildCatalogEntry(resolverProfile.headEntry);
    profile.appendageEntry = BuildCatalogEntry(resolverProfile.appendageEntry);
    profile.overlayEntry = BuildCatalogEntry(resolverProfile.overlayEntry);
    profile.groundingEntry = BuildCatalogEntry(resolverProfile.groundingEntry);

    profile.resolvedEntryCount = CountResolvedEntries(profile);
    profile.brief =
        BuildBrief(profile.catalogState, profile.entryCount, profile.resolvedEntryCount);
    profile.keyBrief = BuildKeyBrief(profile);
    profile.labelBrief = BuildLabelBrief(profile);
    return profile;
}

} // namespace mousefx::windows
