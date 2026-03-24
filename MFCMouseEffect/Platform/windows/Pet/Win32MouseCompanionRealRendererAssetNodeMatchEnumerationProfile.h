#pragma once

#include <cstdint>
#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererSceneRuntime;

struct Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry final {
    std::string logicalNode;
    std::string parserLocator;
    std::string enumerationKey;
    std::string enumerationLabel;
    std::string aliasSeed;
    float enumerationConfidence{0.0f};
    bool resolved{false};
};

struct Win32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile final {
    std::string enumerationState{"preview_only"};
    uint32_t entryCount{0};
    uint32_t resolvedEntryCount{0};
    Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry bodyEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry headEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry appendageEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry overlayEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchEnumerationEntry groundingEntry{};
    std::string brief{"preview_only/0/0"};
    std::string locatorBrief{
        "body:preview://body|head:preview://head|appendage:preview://appendage|overlay:preview://overlay|grounding:preview://grounding"};
    std::string labelBrief{
        "body:body@enumeration|head:head@enumeration|appendage:appendage@enumeration|overlay:overlay@enumeration|grounding:grounding@enumeration"};
};

Win32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile
BuildWin32MouseCompanionRealRendererAssetNodeMatchEnumerationProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

} // namespace mousefx::windows
