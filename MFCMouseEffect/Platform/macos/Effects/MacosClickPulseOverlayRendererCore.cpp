#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseOverlayRendererCore.h"

namespace mousefx::macos_click_pulse {

void ShowClickPulseOverlayOnMain(
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
    const ClickEffectProfile computeProfile{
        profile.normalSizePx,
        profile.textSizePx,
        profile.normalDurationSec,
        profile.textDurationSec,
        profile.closePaddingMs,
        profile.baseOpacity,
        {profile.leftButton.fillArgb, profile.leftButton.strokeArgb, profile.leftButton.glowArgb},
        {profile.rightButton.fillArgb, profile.rightButton.strokeArgb, profile.rightButton.glowArgb},
        {profile.middleButton.fillArgb, profile.middleButton.strokeArgb, profile.middleButton.glowArgb},
    };
    const ClickEffectRenderCommand command =
        ComputeClickEffectRenderCommand(overlayPt, button, effectType, computeProfile);
    ShowClickPulseOverlayOnMain(command, themeName);
#endif
}

} // namespace mousefx::macos_click_pulse
