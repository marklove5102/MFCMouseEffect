#pragma once

#include <cstdint>
#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererModelAssetNodePresenceProfile;
struct Win32MouseCompanionRealRendererModelNodeBindingProfile;
struct Win32MouseCompanionRealRendererSceneRuntime;
struct Win32MouseCompanionRealRendererScene;

struct Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile final {
    uint32_t entryCount{0};
    uint32_t resolvedEntryCount{0};
    float presenceRegistryWeight{0.0f};
    std::string presenceRegistryState{"preview_only"};
    std::string brief{"preview_only/0/0"};
    std::string registryBrief{
        "body:stub|head:stub|appendage:stub|overlay:stub|grounding:stub|adapter:runtime_only"};
    std::string valueBrief{
        "body:0.00|head:0.00|appendage:0.00|overlay:0.00|grounding:0.00|adapter:0.00"};
};

Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodePresenceProfile& nodePresenceProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode);

Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

void ApplyWin32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodePresenceRegistryProfile& profile,
    Win32MouseCompanionRealRendererScene& scene);

} // namespace mousefx::windows
