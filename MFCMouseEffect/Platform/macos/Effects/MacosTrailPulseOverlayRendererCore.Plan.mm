#include "pch.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Internal.h"
#include "Platform/macos/Effects/MacosTrailPulseOverlayStyle.h"

#include <algorithm>

namespace mousefx::macos_trail_pulse {

#if defined(__APPLE__)
namespace {

const macos_effect_profile::TrailRenderProfile::TypeTempoProfile& ResolveTrailTempoProfile(
    const std::string& trailType,
    const macos_effect_profile::TrailRenderProfile& profile) {
    if (trailType == "meteor") return profile.meteorTempo;
    if (trailType == "streamer") return profile.streamerTempo;
    if (trailType == "electric") return profile.electricTempo;
    if (trailType == "tubes") return profile.tubesTempo;
    if (trailType == "particle") return profile.particleTempo;
    return profile.lineTempo;
}

} // namespace

TrailPulseRenderPlan BuildTrailPulseRenderPlan(
    const ScreenPoint& overlayPt,
    const std::string& effectType,
    const macos_effect_profile::TrailRenderProfile& profile) {
    TrailPulseRenderPlan plan{};
    plan.trailType = detail::NormalizeTrailType(effectType);
    plan.tubesMode = (plan.trailType == "tubes");
    plan.particleMode = (plan.trailType == "particle");
    plan.glowMode = (plan.trailType == "meteor" || plan.trailType == "streamer");

    const auto& tempo = ResolveTrailTempoProfile(plan.trailType, profile);
    plan.durationScale = tempo.durationScale;
    const double sizeScale = tempo.sizeScale;
    const CGFloat baseSize = plan.particleMode
        ? static_cast<CGFloat>(profile.particleSizePx)
        : static_cast<CGFloat>(profile.normalSizePx);
    plan.size = static_cast<CGFloat>(std::clamp<double>(baseSize * sizeScale, 28.0, 180.0));
    const NSRect rawFrame = NSMakeRect(
        overlayPt.x - plan.size * 0.5,
        overlayPt.y - plan.size * 0.5,
        plan.size,
        plan.size);
    plan.frame = macos_overlay_support::ClampOverlayFrameToScreenBounds(rawFrame, overlayPt);
    plan.durationSec = macos_overlay_support::ScaleOverlayDurationBySize(
        profile.durationSec * plan.durationScale,
        plan.size,
        160.0,
        0.14,
        1.60);
    plan.closeAfterMs = static_cast<int>(plan.durationSec * 1000.0) + profile.closePaddingMs;
    return plan;
}

#endif

} // namespace mousefx::macos_trail_pulse
