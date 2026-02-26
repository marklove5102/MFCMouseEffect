#include "pch.h"

#include "Platform/macos/Effects/MacosScrollPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosScrollPulseWindowRegistry.h"

namespace mousefx::macos_scroll_pulse {

void CloseAllScrollPulseWindows() {
#if !defined(__APPLE__)
    return;
#else
    macos_overlay_support::RunOnMainThreadSync(^{
      CloseAllScrollPulseWindowsNow();
    });
#endif
}

void ShowScrollPulseOverlay(
    const ScreenPoint& overlayPt,
    bool horizontal,
    int delta,
    const std::string& effectType,
    const std::string& themeName,
    const macos_effect_profile::ScrollRenderProfile& profile) {
#if !defined(__APPLE__)
    (void)overlayPt;
    (void)horizontal;
    (void)delta;
    (void)effectType;
    (void)themeName;
    (void)profile;
    return;
#else
    if (delta == 0) {
        return;
    }

    const ScreenPoint ptCopy = overlayPt;
    const bool horizontalCopy = horizontal;
    const int deltaCopy = delta;
    const std::string typeCopy = effectType;
    const std::string themeCopy = themeName;
    const macos_effect_profile::ScrollRenderProfile profileCopy = profile;
    macos_overlay_support::RunOnMainThreadAsync(^{
      ShowScrollPulseOverlayOnMain(ptCopy, horizontalCopy, deltaCopy, typeCopy, themeCopy, profileCopy);
    });
#endif
}

void ShowScrollPulseOverlay(
    const ScreenPoint& overlayPt,
    bool horizontal,
    int delta,
    const std::string& effectType,
    const std::string& themeName) {
    ShowScrollPulseOverlay(overlayPt, horizontal, delta, effectType, themeName, macos_effect_profile::DefaultScrollRenderProfile());
}

} // namespace mousefx::macos_scroll_pulse
