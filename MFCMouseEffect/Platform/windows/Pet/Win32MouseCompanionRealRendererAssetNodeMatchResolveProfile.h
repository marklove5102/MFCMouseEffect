#pragma once

#include <cstdint>
#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererSceneRuntime;

struct Win32MouseCompanionRealRendererAssetNodeMatchResolveEntry final {
    std::string logicalNode;
    std::string parserLocator;
    std::string probeKey;
    std::string probeLabel;
    std::string finalNodeKey;
    std::string finalNodeLabel;
    std::string routeState;
    float resolveConfidence{0.0f};
    bool resolved{false};
};

struct Win32MouseCompanionRealRendererAssetNodeMatchResolveProfile final {
    std::string resolveState{"preview_only"};
    uint32_t entryCount{0};
    uint32_t resolvedEntryCount{0};
    Win32MouseCompanionRealRendererAssetNodeMatchResolveEntry bodyEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchResolveEntry headEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchResolveEntry appendageEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchResolveEntry overlayEntry{};
    Win32MouseCompanionRealRendererAssetNodeMatchResolveEntry groundingEntry{};
    std::string brief{"preview_only/0/0"};
    std::string keyBrief{
        "body:body|head:head|appendage:appendage|overlay:overlay|grounding:grounding"};
    std::string labelBrief{
        "body:body@resolve|head:head@resolve|appendage:appendage@resolve|overlay:overlay@resolve|grounding:grounding@resolve"};
};

Win32MouseCompanionRealRendererAssetNodeMatchResolveProfile
BuildWin32MouseCompanionRealRendererAssetNodeMatchResolveProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

} // namespace mousefx::windows
