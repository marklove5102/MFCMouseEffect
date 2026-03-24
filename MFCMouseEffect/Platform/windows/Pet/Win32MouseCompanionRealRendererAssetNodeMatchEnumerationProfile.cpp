#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

std::string ResolveEnumerationState(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    const std::string& catalogState = runtime.assetNodeMatchCatalogProfile.catalogState;
    if (catalogState == "match_catalog_ready") {
        return "match_enumeration_ready";
    }
    if (catalogState == "match_catalog_stub_ready") {
        return "match_enumeration_stub_ready";
    }
    if (catalogState == "match_catalog_scaffold") {
        return "match_enumeration_scaffold";
    }
    return "preview_only";
}

std::string ResolveParserLocator(
    const Win32MouseCompanionRealRendererAssetNodeMatchCandidateEntry& candidateEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry& catalogEntry) {
    if (!candidateEntry.candidatePath.empty()) {
        return "parser://" + candidateEntry.candidatePath;
    }
    if (!catalogEntry.canonicalNodeKey.empty()) {
        return "parser://" + catalogEntry.canonicalNodeKey;
    }
    return "preview://" + catalogEntry.logicalNode;
}

std::string ResolveEnumerationKey(
    const Win32MouseCompanionRealRendererAssetNodeMatchCandidateEntry& candidateEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry& catalogEntry) {
    if (!catalogEntry.canonicalNodeKey.empty()) {
        return catalogEntry.canonicalNodeKey;
    }
    if (!candidateEntry.candidateTokenSeed.empty()) {
        return catalogEntry.logicalNode + "|" + candidateEntry.candidateTokenSeed;
    }
    return catalogEntry.logicalNode + "|enumeration";
}

std::string ResolveEnumerationLabel(
    const Win32MouseCompanionRealRendererAssetNodeMatchCandidateEntry& candidateEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry& catalogEntry) {
    if (!catalogEntry.canonicalNodeLabel.empty()) {
        return catalogEntry.canonicalNodeLabel + "@enumeration";
    }
    if (!candidateEntry.primaryCandidateName.empty()) {
        return catalogEntry.logicalNode + "@" + candidateEntry.primaryCandidateName + "@enumeration";
    }
    return catalogEntry.logicalNode + "@enumeration";
}

std::string ResolveAliasSeed(
    const Win32MouseCompanionRealRendererAssetNodeMatchCandidateEntry& candidateEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry& catalogEntry) {
    if (!candidateEntry.candidateTokenSeed.empty()) {
        return candidateEntry.candidateTokenSeed;
    }
    if (!catalogEntry.candidateTokenSeed.empty()) {
        return catalogEntry.candidateTokenSeed;
    }
    return catalogEntry.logicalNode + ":alias";
}

Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry BuildEnumerationEntry(
    const Win32MouseCompanionRealRendererAssetNodeMatchCandidateEntry& candidateEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry& catalogEntry) {
    Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry entry{};
    entry.logicalNode = catalogEntry.logicalNode;
    entry.parserLocator = ResolveParserLocator(candidateEntry, catalogEntry);
    entry.enumerationKey = ResolveEnumerationKey(candidateEntry, catalogEntry);
    entry.enumerationLabel = ResolveEnumerationLabel(candidateEntry, catalogEntry);
    entry.aliasSeed = ResolveAliasSeed(candidateEntry, catalogEntry);
    entry.enumerationConfidence = std::clamp(
        catalogEntry.matchConfidence +
            (!candidateEntry.secondaryCandidateName.empty() ? 0.06f : 0.0f) +
            (!candidateEntry.selectorPrefix.empty() ? 0.04f : 0.0f),
        0.0f,
        1.0f);
    entry.resolved = catalogEntry.resolved;
    return entry;
}

uint32_t CountResolvedEntries(
    const Win32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile& profile) {
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
    const Win32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile& profile) {
    char buffer[1024];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.parserLocator.c_str(),
        profile.headEntry.parserLocator.c_str(),
        profile.appendageEntry.parserLocator.c_str(),
        profile.overlayEntry.parserLocator.c_str(),
        profile.groundingEntry.parserLocator.c_str());
    return std::string(buffer);
}

std::string BuildLabelBrief(
    const Win32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile& profile) {
    char buffer[1024];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.enumerationLabel.c_str(),
        profile.headEntry.enumerationLabel.c_str(),
        profile.appendageEntry.enumerationLabel.c_str(),
        profile.overlayEntry.enumerationLabel.c_str(),
        profile.groundingEntry.enumerationLabel.c_str());
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile
BuildWin32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile profile{};
    profile.enumerationState = ResolveEnumerationState(runtime);
    profile.entryCount = 5;

    const auto& candidateProfile = runtime.assetNodeMatchCandidateProfile;
    const auto& catalogProfile = runtime.assetNodeMatchCatalogProfile;
    profile.bodyEntry = BuildEnumerationEntry(candidateProfile.bodyEntry, catalogProfile.bodyEntry);
    profile.headEntry = BuildEnumerationEntry(candidateProfile.headEntry, catalogProfile.headEntry);
    profile.appendageEntry =
        BuildEnumerationEntry(candidateProfile.appendageEntry, catalogProfile.appendageEntry);
    profile.overlayEntry =
        BuildEnumerationEntry(candidateProfile.overlayEntry, catalogProfile.overlayEntry);
    profile.groundingEntry =
        BuildEnumerationEntry(candidateProfile.groundingEntry, catalogProfile.groundingEntry);

    profile.resolvedEntryCount = CountResolvedEntries(profile);
    profile.brief =
        BuildBrief(profile.enumerationState, profile.entryCount, profile.resolvedEntryCount);
    profile.locatorBrief = BuildLocatorBrief(profile);
    profile.labelBrief = BuildLabelBrief(profile);
    return profile;
}

} // namespace mousefx::windows
