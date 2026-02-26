#include "pch.h"

#include "Platform/macos/Effects/MacosHoverPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"

namespace mousefx::macos_hover_pulse {

void ShowHoverPulseOverlay(
    const ScreenPoint& overlayPt,
    const std::string& effectType,
    const std::string& themeName,
    const macos_effect_profile::HoverRenderProfile& profile) {
#if !defined(__APPLE__)
    (void)overlayPt;
    (void)effectType;
    (void)themeName;
    (void)profile;
    return;
#else
    const ScreenPoint ptCopy = overlayPt;
    const std::string typeCopy = effectType;
    const std::string themeCopy = themeName;
    const macos_effect_profile::HoverRenderProfile profileCopy = profile;
    macos_overlay_support::RunOnMainThreadAsync(^{
      ShowHoverPulseOverlayOnMain(ptCopy, typeCopy, themeCopy, profileCopy);
    });
#endif
}

void ShowHoverPulseOverlay(
    const ScreenPoint& overlayPt,
    const std::string& effectType,
    const std::string& themeName) {
    ShowHoverPulseOverlay(overlayPt, effectType, themeName, macos_effect_profile::DefaultHoverRenderProfile());
}

void CloseHoverPulseOverlay() {
#if !defined(__APPLE__)
    return;
#else
    macos_overlay_support::RunOnMainThreadSync(^{
      CloseHoverPulseOverlayOnMain();
    });
#endif
}

size_t GetActiveHoverPulseWindowCount() {
#if !defined(__APPLE__)
    return 0;
#else
    __block size_t count = 0;
    macos_overlay_support::RunOnMainThreadSync(^{
      count = GetActiveHoverPulseWindowCountOnMain();
    });
    return count;
#endif
}

} // namespace mousefx::macos_hover_pulse
