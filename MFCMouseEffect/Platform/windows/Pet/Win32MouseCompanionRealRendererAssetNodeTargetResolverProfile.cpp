#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererAssetNodeTargetResolverProfile.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererSceneRuntime.h"

#include <algorithm>
#include <cstdio>

namespace mousefx::windows {
namespace {

std::string ResolveTargetResolverState(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    const std::string& targetState = runtime.assetNodeTargetProfile.targetState;
    if (targetState == "target_ready") {
        return "target_resolver_ready";
    }
    if (targetState == "target_stub_ready") {
        return "target_resolver_stub_ready";
    }
    if (targetState == "target_scaffold") {
        return "target_resolver_scaffold";
    }
    return "preview_only";
}

float ResolveTargetResolverWeight(
    const std::string& logicalNode,
    float targetWeight,
    float resolverWeight) {
    const float combined = targetWeight * 0.62f + resolverWeight * 0.38f;
    if (logicalNode == "head") {
        return combined * 1.02f;
    }
    if (logicalNode == "appendage") {
        return combined * 1.04f;
    }
    if (logicalNode == "overlay") {
        return combined * 0.97f;
    }
    if (logicalNode == "grounding") {
        return combined * 0.95f;
    }
    return combined;
}

float ResolveMatchConfidence(
    const Win32MouseCompanionRealRendererAssetNodeTargetEntry& targetEntry,
    const Win32MouseCompanionRealRendererAssetNodeResolverEntry& resolverEntry,
    float resolvedWeight) {
    float confidence = resolvedWeight;
    if (!targetEntry.selectorKey.empty() && targetEntry.selectorKey.find('|') != std::string::npos) {
        confidence += 0.10f;
    }
    if (!targetEntry.candidateNodeName.empty() && targetEntry.candidateNodeName != "unknown") {
        confidence += 0.08f;
    }
    if (!resolverEntry.assetNodePath.empty() && resolverEntry.assetNodePath.find("/asset/") != std::string::npos) {
        confidence += 0.06f;
    }
    return std::clamp(confidence, 0.0f, 1.0f);
}

std::string ResolveResolvedNodeKey(
    const Win32MouseCompanionRealRendererAssetNodeTargetEntry& targetEntry) {
    if (!targetEntry.selectorKey.empty()) {
        return targetEntry.selectorKey;
    }
    if (!targetEntry.candidateNodeName.empty()) {
        return targetEntry.logicalNode + "|" + targetEntry.candidateNodeName;
    }
    return targetEntry.logicalNode + "|unknown";
}

std::string ResolveResolvedNodeLabel(
    const Win32MouseCompanionRealRendererAssetNodeTargetEntry& targetEntry,
    float matchConfidence) {
    const std::string name =
        targetEntry.candidateNodeName.empty() ? "unknown" : targetEntry.candidateNodeName;
    char buffer[160];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s@%s#%.2f",
        targetEntry.logicalNode.c_str(),
        name.c_str(),
        matchConfidence);
    return std::string(buffer);
}

Win32MouseCompanionRealRendererAssetNodeTargetResolverEntry BuildTargetResolverEntry(
    const Win32MouseCompanionRealRendererAssetNodeTargetEntry& targetEntry,
    const Win32MouseCompanionRealRendererAssetNodeResolverEntry& resolverEntry) {
    Win32MouseCompanionRealRendererAssetNodeTargetResolverEntry entry{};
    entry.logicalNode = targetEntry.logicalNode;
    entry.parentLogicalNode = resolverEntry.parentLogicalNode;
    entry.modelNodePath = targetEntry.modelNodePath;
    entry.assetNodePath = resolverEntry.assetNodePath;
    entry.targetKind = targetEntry.targetKind;
    entry.sourceTag = targetEntry.sourceTag;
    entry.selectorKey = targetEntry.selectorKey;
    entry.candidateNodeName = targetEntry.candidateNodeName;
    entry.resolvedWeight = ResolveTargetResolverWeight(
        targetEntry.logicalNode,
        targetEntry.targetWeight,
        resolverEntry.resolvedWeight);
    entry.matchConfidence =
        ResolveMatchConfidence(targetEntry, resolverEntry, entry.resolvedWeight);
    entry.resolvedNodeKey = ResolveResolvedNodeKey(targetEntry);
    entry.resolvedNodeLabel =
        ResolveResolvedNodeLabel(targetEntry, entry.matchConfidence);
    entry.resolvedOffsetX =
        targetEntry.targetOffsetX * (0.68f + entry.resolvedWeight * 0.22f);
    entry.resolvedOffsetY =
        targetEntry.targetOffsetY * (0.70f + entry.resolvedWeight * 0.20f);
    entry.resolvedScale =
        1.0f + (targetEntry.targetScale - 1.0f) * (0.82f + entry.resolvedWeight * 0.18f);
    entry.resolved =
        targetEntry.resolved && resolverEntry.resolved && entry.resolvedWeight > 0.0f;
    return entry;
}

uint32_t CountResolvedEntries(
    const Win32MouseCompanionRealRendererAssetNodeTargetResolverProfile& profile) {
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

std::string BuildPathBrief(
    const Win32MouseCompanionRealRendererAssetNodeTargetResolverProfile& profile) {
    char buffer[320];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:%s|head:%s|appendage:%s|overlay:%s|grounding:%s",
        profile.bodyEntry.assetNodePath.c_str(),
        profile.headEntry.assetNodePath.c_str(),
        profile.appendageEntry.assetNodePath.c_str(),
        profile.overlayEntry.assetNodePath.c_str(),
        profile.groundingEntry.assetNodePath.c_str());
    return std::string(buffer);
}

std::string BuildValueBrief(
    const Win32MouseCompanionRealRendererAssetNodeTargetResolverProfile& profile) {
    char buffer[320];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "body:(%.2f,%.2f,%.2f)|head:(%.2f,%.2f,%.2f)|appendage:(%.2f,%.2f,%.2f)|overlay:(%.2f,%.2f,%.2f)|grounding:(%.2f,%.2f,%.2f)",
        profile.bodyEntry.resolvedOffsetX,
        profile.bodyEntry.resolvedOffsetY,
        profile.bodyEntry.resolvedScale,
        profile.headEntry.resolvedOffsetX,
        profile.headEntry.resolvedOffsetY,
        profile.headEntry.resolvedScale,
        profile.appendageEntry.resolvedOffsetX,
        profile.appendageEntry.resolvedOffsetY,
        profile.appendageEntry.resolvedScale,
        profile.overlayEntry.resolvedOffsetX,
        profile.overlayEntry.resolvedOffsetY,
        profile.overlayEntry.resolvedScale,
        profile.groundingEntry.resolvedOffsetX,
        profile.groundingEntry.resolvedOffsetY,
        profile.groundingEntry.resolvedScale);
    return std::string(buffer);
}

} // namespace

Win32MouseCompanionRealRendererAssetNodeTargetResolverProfile
BuildWin32MouseCompanionRealRendererAssetNodeTargetResolverProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererAssetNodeTargetResolverProfile profile{};
    profile.resolverState = ResolveTargetResolverState(runtime);
    profile.entryCount = 5;

    const auto& target = runtime.assetNodeTargetProfile;
    const auto& resolver = runtime.assetNodeResolverProfile;
    profile.bodyEntry = BuildTargetResolverEntry(target.bodyEntry, resolver.bodyEntry);
    profile.headEntry = BuildTargetResolverEntry(target.headEntry, resolver.headEntry);
    profile.appendageEntry = BuildTargetResolverEntry(target.appendageEntry, resolver.appendageEntry);
    profile.overlayEntry = BuildTargetResolverEntry(target.overlayEntry, resolver.overlayEntry);
    profile.groundingEntry = BuildTargetResolverEntry(target.groundingEntry, resolver.groundingEntry);

    profile.resolvedEntryCount = CountResolvedEntries(profile);
    profile.brief = BuildBrief(profile.resolverState, profile.entryCount, profile.resolvedEntryCount);
    profile.pathBrief = BuildPathBrief(profile);
    profile.valueBrief = BuildValueBrief(profile);
    return profile;
}

} // namespace mousefx::windows
