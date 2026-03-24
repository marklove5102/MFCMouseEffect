#pragma once

#include <cstdint>
#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererSceneRuntime;

struct Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry final {
    std::string logicalNode;
    std::string parserLocator;
    std::string probeKey;
    std::string probeLabel;
    std::string fallbackAlias;
    std::string planTokenSeed;
    float planConfidence{0.0f};
    bool resolved{false};
};

struct Win32MouseCompanionRealRendererAssetNodeMatchPlanProfile final {
    std::string planState{"preview_only"};
    uint32_t entryCount{0};
    uint32_t resolvedEntryCount{0};
    Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry bodyEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry headEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry appendageEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry overlayEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchPlanEntry groundingEntry{};
    std::string brief{"preview_only/0/0"};
    std::string locatorBrief{
        "body:preview://body|head:preview://head|appendage:preview://appendage|overlay:preview://overlay|grounding:preview://grounding"};
    std::string valueBrief{
        "body:body@plan|head:head@plan|appendage:appendage@plan|overlay:overlay@plan|grounding:grounding@plan"};
};

Win32MouseCompanionRealRendererAssetNodeMatchPlanProfile
BuildWin32MouseCompanionRealRendererAssetNodeMatchPlanProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

} // namespace mousefx::windows
