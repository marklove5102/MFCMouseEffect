#include "pch.h"

#include "Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.h"

namespace mousefx::macos_trail_pulse {

void ShowTrailPulseOverlayOnMain(
    const ScreenPoint& overlayPt,
    double deltaX,
    double deltaY,
    const std::string& effectType,
    const std::string& themeName,
    const macos_effect_profile::TrailRenderProfile& profile) {
#if !defined(__APPLE__)
    (void)overlayPt;
    (void)deltaX;
    (void)deltaY;
    (void)effectType;
    (void)themeName;
    (void)profile;
    return;
#else
    const TrailEffectProfile computeProfile{
        profile.normalSizePx,
        profile.particleSizePx,
        profile.durationSec,
        profile.closePaddingMs,
        profile.baseOpacity,
        {profile.line.fillArgb, profile.line.strokeArgb},
        {profile.streamer.fillArgb, profile.streamer.strokeArgb},
        {profile.electric.fillArgb, profile.electric.strokeArgb},
        {profile.meteor.fillArgb, profile.meteor.strokeArgb},
        {profile.tubes.fillArgb, profile.tubes.strokeArgb},
        {profile.particle.fillArgb, profile.particle.strokeArgb},
        {profile.lineTempo.durationScale, profile.lineTempo.sizeScale},
        {profile.streamerTempo.durationScale, profile.streamerTempo.sizeScale},
        {profile.electricTempo.durationScale, profile.electricTempo.sizeScale},
        {profile.meteorTempo.durationScale, profile.meteorTempo.sizeScale},
        {profile.tubesTempo.durationScale, profile.tubesTempo.sizeScale},
        {profile.particleTempo.durationScale, profile.particleTempo.sizeScale},
    };
    const TrailEffectRenderCommand command =
        ComputeTrailEffectRenderCommand(overlayPt, deltaX, deltaY, effectType, computeProfile);
    ShowTrailPulseOverlayOnMain(command, themeName);
#endif
}

} // namespace mousefx::macos_trail_pulse
