#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeMatchPlanProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

std::string ResolvePlanState(const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    const std::string& enumerationState =
        runtime.assetNodeMatchEnumerationProfile.enumerationState;
    if (enumerationState == "match_enumeration_ready") {
        return "match_plan_ready";
    }
    if (enumerationState == "match_enumeration_stub_ready") {
        return "match_plan_stub_ready";
    }
    if (enumerationState == "match_enumeration_scaffold") {
        return "match_plan_scaffold";
    }
    return "preview_only";
}

std::string ResolveFallbackAlias(
    const Win32MouseCompanionRealRendererAssetNodeMatchCandidateEntry& candidateEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry& enumerationEntry) {
    if (!candidateEntry.secondaryCandidateName.empty()) {
        return candidateEntry.secondaryCandidateName;
    }
    if (!enumerationEntry.aliasSeed.empty()) {
        return enumerationEntry.aliasSeed;
    }
    return enumerationEntry.logicalNode + "_fallback";
}

std::string ResolveProbeKey(
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry& catalogEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry& enumerationEntry) {
    if (!enumerationEntry.enumerationKey.empty()) {
        return enumerationEntry.enumerationKey;
    }
    if (!catalogEntry.canonicalNodeKey.empty()) {
        return catalogEntry.canonicalNodeKey;
    }
    return enumerationEntry.logicalNode + "|probe";
}

std::string ResolveProbeLabel(
    const Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry& enumerationEntry,
    const std::string& fallbackAlias) {
    if (!enumerationEntry.enumerationLabel.empty()) {
        return enumerationEntry.enumerationLabel + "@" + fallbackAlias;
    }
    return enumerationEntry.logicalNode + "@plan";
}

Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry BuildPlanEntry(
    const Win32MouseCompanionRealRendererAssetNodeMatchCandidateEntry& candidateEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchCatalogEntry& catalogEntry,
    const Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry& enumerationEntry) {
    Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry entry{};
    entry.logicalNode = enumerationEntry.logicalNode;
    entry.parserLocator = enumerationEntry.parserLocator;
    entry.fallbackAlias = ResolveFallbackAlias(candidateEntry, enumerationEntry);
    entry.probeKey = ResolveProbeKey(catalogEntry, enumerationEntry);
    entry.probeLabel = ResolveProbeLabel(enumerationEntry, entry.fallbackAlias);
    entry.planTokenSeed =
        candidateEntry.candidateTokenSeed.empty()
            ? enumerationEntry.aliasSeed
            : candidateEntry.candidateTokenSeed;
    entry.planConfidence = std::clamp(
        enumerationEntry.enumerationConfidence +
            (!entry.fallbackAlias.empty() ? 0.05f : 0.0f) +
            (!entry.planTokenSeed.empty() ? 0.03f : 0.0f),
        0.0f,
        1.0f);
    entry.resolved = enumerationEntry.resolved;
    return entry;
}

uint32_t CountResolvedEntries(
    const Win32MouseCompanionRealRendererAssetNodeMatchPlanProfile& profile) {
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
    const Win32MouseCompanionRealRendererAssetNodeMatchPlanProfile& profile) {
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

std::string BuildValueBrief(
    const Win32MouseCompanionRealRendererAssetNodeMatchPlanProfile& profile) {
    char buffer[1024];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.probeLabel.c_str(),
        profile.headEntry.probeLabel.c_str(),
        profile.appendageEntry.probeLabel.c_str(),
        profile.overlayEntry.probeLabel.c_str(),
        profile.groundingEntry.probeLabel.c_str());
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererAssetNodeMatchPlanProfile
BuildWin32MouseCompanionRealRendererAssetNodeMatchPlanProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererAssetNodeMatchPlanProfile profile{};
    profile.planState = ResolvePlanState(runtime);
    profile.entryCount = 5;

    const auto& candidateProfile = runtime.assetNodeMatchCandidateProfile;
    const auto& catalogProfile = runtime.assetNodeMatchCatalogProfile;
    const auto& enumerationProfile = runtime.assetNodeMatchEnumerationProfile;

    profile.bodyEntry = BuildPlanEntry(
        candidateProfile.bodyEntry,
        catalogProfile.bodyEntry,
        enumerationProfile.bodyEntry);
    profile.headEntry = BuildPlanEntry(
        candidateProfile.headEntry,
        catalogProfile.headEntry,
        enumerationProfile.headEntry);
    profile.appendageEntry = BuildPlanEntry(
        candidateProfile.appendageEntry,
        catalogProfile.appendageEntry,
        enumerationProfile.appendageEntry);
    profile.overlayEntry = BuildPlanEntry(
        candidateProfile.overlayEntry,
        catalogProfile.overlayEntry,
        enumerationProfile.overlayEntry);
    profile.groundingEntry = BuildPlanEntry(
        candidateProfile.groundingEntry,
        catalogProfile.groundingEntry,
        enumerationProfile.groundingEntry);

    profile.resolvedEntryCount = CountResolvedEntries(profile);
    profile.brief = BuildBrief(profile.planState, profile.entryCount, profile.resolvedEntryCount);
    profile.locatorBrief = BuildLocatorBrief(profile);
    profile.valueBrief = BuildValueBrief(profile);
    return profile;
}

} // namespace mousefx::windows
