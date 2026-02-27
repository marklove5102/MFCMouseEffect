#include "pch.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Internal.h"
#include "Platform/macos/Effects/MacosTrailPulseOverlayStyle.h"

#include <algorithm>

namespace mousefx::macos_trail_pulse {

#if defined(__APPLE__)
namespace {

double ResolveTrailDurationScale(const std::string& trailType) {
    if (trailType == "meteor") return 1.18;
    if (trailType == "streamer") return 0.94;
    if (trailType == "electric") return 0.82;
    if (trailType == "tubes") return 1.06;
    if (trailType == "particle") return 0.78;
    return 1.0;
}

double ResolveTrailSizeScale(const std::string& trailType) {
    if (trailType == "meteor") return 1.08;
    if (trailType == "electric") return 0.92;
    if (trailType == "tubes") return 1.05;
    if (trailType == "particle") return 0.86;
    return 1.0;
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

    plan.durationScale = ResolveTrailDurationScale(plan.trailType);
    const double sizeScale = ResolveTrailSizeScale(plan.trailType);
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
