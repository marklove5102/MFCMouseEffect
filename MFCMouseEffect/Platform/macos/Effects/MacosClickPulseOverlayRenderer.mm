#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosClickPulseOverlayRendererCore.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosClickPulseWindowRegistry.h"

namespace mousefx::macos_click_pulse {

void CloseAllClickPulseWindows() {
#if !defined(__APPLE__)
    return;
#else
    macos_overlay_support::RunOnMainThreadSync(^{
      CloseAllClickPulseWindowsNow();
    });
#endif
}

void ShowClickPulseOverlay(
    const ScreenPoint& overlayPt,
    MouseButton button,
    const std::string& effectType,
    const std::string& themeName,
    const macos_effect_profile::ClickRenderProfile& profile) {
#if !defined(__APPLE__)
    (void)overlayPt;
    (void)button;
    (void)effectType;
    (void)themeName;
    (void)profile;
    return;
#else
    const ScreenPoint ptCopy = overlayPt;
    const MouseButton buttonCopy = button;
    const std::string typeCopy = effectType;
    const std::string themeCopy = themeName;
    const macos_effect_profile::ClickRenderProfile profileCopy = profile;
    macos_overlay_support::RunOnMainThreadAsync(^{
      ShowClickPulseOverlayOnMain(ptCopy, buttonCopy, typeCopy, themeCopy, profileCopy);
    });
#endif
}

void ShowClickPulseOverlay(
    const ScreenPoint& overlayPt,
    MouseButton button,
    const std::string& effectType,
    const std::string& themeName) {
    ShowClickPulseOverlay(overlayPt, button, effectType, themeName, macos_effect_profile::DefaultClickRenderProfile());
}

} // namespace mousefx::macos_click_pulse
