#include "pch.h"

#include "Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.h"

namespace mousefx::macos_hover_pulse {

void ShowHoverPulseOverlayOnMain(
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
    const HoverEffectProfile computeProfile{
        profile.sizePx,
        profile.breatheDurationSec,
        profile.spinDurationSec,
        profile.baseOpacity,
        profile.glowSizeScale,
        profile.tubesSizeScale,
        profile.glowBreatheScale,
        profile.tubesBreatheScale,
        profile.tubesSpinScale,
        {profile.colors.glowFillArgb, profile.colors.glowStrokeArgb, profile.colors.tubesStrokeArgb},
    };
    const HoverEffectRenderCommand command = ComputeHoverEffectRenderCommand(overlayPt, effectType, computeProfile);
    ShowHoverPulseOverlayOnMain(command, themeName);
#endif
}

} // namespace mousefx::macos_hover_pulse
