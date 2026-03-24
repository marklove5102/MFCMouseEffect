#pragma once

#include <cstdint>
#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile;
struct Win32MouseCompanionRealRendererModelNodeBindingProfile;
struct Win32MouseCompanionRealRendererSceneRuntime;
struct Win32MouseCompanionRealRendererScene;

struct Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile final {
    uint32_t entryCount{0};
    uint32_t resolvedEntryCount{0};
    float consumerRegistryWeight{0.0f};
    std::string consumerRegistryState{"preview_only"};
    std::string brief{"preview_only/0/0"};
    std::string registryBrief{
        "body:stub|head:stub|appendage:stub|overlay:stub|grounding:stub|adapter:runtime_only"};
    std::string valueBrief{
        "body:0.00|head:0.00|appendage:0.00|overlay:0.00|grounding:0.00|adapter:0.00"};
};

Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile& nodeConsumerProfile,
    const Win32MouseCompanionRealRendererModelNodeBindingProfile& nodeBindingProfile,
    const std::string& adapterMode);

Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

void ApplyWin32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeConsumerRegistryProfile& profile,
    Win32MouseCompanionRealRendererScene& scene);

} // namespace mousefx::windows
