#pragma once

#include <cstdint>
#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererModelAssetNodeDriverRegistryProfile;
struct Win32MouseCompanionRealRendererModelNodeRegistryProfile;
struct Win32MouseCompanionRealRendererSceneRuntime;
struct Win32MouseCompanionRealRendererScene;

struct Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile final {
    uint32_t entryCount{0};
    uint32_t resolvedEntryCount{0};
    float consumerWeight{0.0f};
    std::string consumerState{"preview_only"};
    std::string brief{"preview_only/0/0"};
    std::string consumerBrief{
        "body:stub|head:stub|appendage:stub|overlay:stub|grounding:stub|adapter:runtime_only"};
    std::string valueBrief{
        "body:0.00|head:0.00|appendage:0.00|overlay:0.00|grounding:0.00|adapter:0.00"};
};

Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeDriverRegistryProfile& nodeDriverRegistryProfile,
    const Win32MouseCompanionRealRendererModelNodeRegistryProfile& nodeRegistryProfile,
    const std::string& adapterMode);

Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile
BuildWin32MouseCompanionRealRendererModelAssetNodeConsumerProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

void ApplyWin32MouseCompanionRealRendererModelAssetNodeConsumerProfile(
    const Win32MouseCompanionRealRendererModelAssetNodeConsumerProfile& profile,
    Win32MouseCompanionRealRendererScene& scene);

} // namespace mousefx::windows
