#pragma once

#include <cstdint>
#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererSceneRuntime;

struct Win32MouseCompanionRealRendererModelNodeSlotEntry final {
    std::string logicalNode;
    std::string slotName;
    float bindWeight{0.0f};
    bool slotReady{false};
};

struct Win32MouseCompanionRealRendererModelNodeSlotProfile final {
    std::string slotState{"preview_only"};
    uint32_t slotCount{0};
    uint32_t readySlotCount{0};
    Win32MouseCompanionRealRendererModelNodeSlotEntry bodySlot{};
    Win32MouseCompanionRealRendererModelNodeSlotEntry headSlot{};
    Win32MouseCompanionRealRendererModelNodeSlotEntry appendageSlot{};
    Win32MouseCompanionRealRendererModelNodeSlotEntry overlaySlot{};
    Win32MouseCompanionRealRendererModelNodeSlotEntry groundingSlot{};
    std::string brief{"preview_only/0/0"};
    std::string slotBrief{
        "body:body_root|head:head_anchor|appendage:appendage_anchor|overlay:overlay_anchor|grounding:grounding_anchor"};
};

Win32MouseCompanionRealRendererModelNodeSlotProfile
BuildWin32MouseCompanionRealRendererModelNodeSlotProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime);

} // namespace mousefx::windows
