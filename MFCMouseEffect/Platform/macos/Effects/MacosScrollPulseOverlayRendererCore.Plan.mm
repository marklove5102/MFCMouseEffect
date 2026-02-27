#include "pch.h"

#include "Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.Internal.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosScrollPulseOverlayRendererSupport.h"
#include "Platform/macos/Effects/MacosScrollPulseOverlayStyle.h"

#include <algorithm>

namespace mousefx::macos_scroll_pulse {

#if defined(__APPLE__)
namespace {

double ResolveScrollDurationScale(
    const ScrollPulseRenderPlan& plan,
    const macos_effect_profile::ScrollRenderProfile& profile) {
    if (plan.helixMode) return profile.helixDurationScale;
    if (plan.twinkleMode) return profile.twinkleDurationScale;
    return profile.defaultDurationScale;
}

double ResolveScrollSizeScale(
    const ScrollPulseRenderPlan& plan,
    const macos_effect_profile::ScrollRenderProfile& profile) {
    if (plan.helixMode) return profile.helixSizeScale;
    if (plan.twinkleMode) return profile.twinkleSizeScale;
    return profile.defaultSizeScale;
}

} // namespace

ScrollPulseRenderPlan BuildScrollPulseRenderPlan(
    const ScreenPoint& overlayPt,
    bool horizontal,
    int delta,
    const std::string& effectType,
    const macos_effect_profile::ScrollRenderProfile& profile) {
    ScrollPulseRenderPlan plan{};
    plan.normalizedType = NormalizeScrollType(effectType);
    plan.helixMode = (plan.normalizedType == "helix");
    plan.twinkleMode = (plan.normalizedType == "twinkle");
    plan.strengthLevel = support::ResolveStrengthLevel(delta);
    plan.durationScale = ResolveScrollDurationScale(plan, profile);

    const CGFloat baseSize = horizontal
        ? static_cast<CGFloat>(profile.horizontalSizePx)
        : static_cast<CGFloat>(profile.verticalSizePx);
    const double sizeScale = ResolveScrollSizeScale(plan, profile);
    plan.size = static_cast<CGFloat>(std::clamp<double>(baseSize * sizeScale, 88.0, 260.0));
    const NSRect rawFrame = NSMakeRect(
        overlayPt.x - plan.size * 0.5,
        overlayPt.y - plan.size * 0.5,
        plan.size,
        plan.size);
    plan.frame = macos_overlay_support::ClampOverlayFrameToScreenBounds(rawFrame, overlayPt);
    plan.bodyRect = support::BuildBodyRect(plan.size, horizontal, plan.strengthLevel);
    plan.duration = support::BuildPulseDuration(profile, plan.strengthLevel, plan.size) * plan.durationScale;
    plan.closeAfterMs = support::BuildCloseAfterMs(profile, plan.duration);
    return plan;
}

#endif

} // namespace mousefx::macos_scroll_pulse
