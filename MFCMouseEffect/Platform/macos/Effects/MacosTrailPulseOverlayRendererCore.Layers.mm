#include "pch.h"

#include "Platform/macos/Effects/MacosTrailPulseOverlayRendererCore.Internal.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosTrailPulseOverlayStyle.h"

namespace mousefx::macos_trail_pulse {

#if defined(__APPLE__)
void ConfigureTrailCoreLayer(
    CAShapeLayer* core,
    NSView* content,
    const TrailPulseRenderPlan& plan,
    double deltaX,
    double deltaY,
    const macos_effect_profile::TrailRenderProfile& profile) {
    core.frame = content.bounds;
    const CGFloat size = CGRectGetWidth(content.bounds);

    if (plan.tubesMode) {
        const CGFloat outerInset = macos_overlay_support::ScaleOverlayMetric(size, 9.0, 160.0, 5.0, 20.0);
        CGPathRef outer = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, outerInset, outerInset), nullptr);
        core.path = outer;
        CGPathRelease(outer);
        core.fillColor = [NSColor clearColor].CGColor;
        core.strokeColor = [detail::TrailStrokeColor(plan.trailType) CGColor];
        core.lineWidth = macos_overlay_support::ScaleOverlayMetric(size, 3.2, 160.0, 1.2, 6.0);
    } else if (plan.particleMode) {
        const CGFloat dotInset = macos_overlay_support::ScaleOverlayMetric(size, 16.0, 160.0, 8.0, 34.0);
        CGPathRef dot = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, dotInset, dotInset), nullptr);
        core.path = dot;
        CGPathRelease(dot);
        core.fillColor = [detail::TrailStrokeColor(plan.trailType) CGColor];
        core.strokeColor = [detail::TrailStrokeColor(plan.trailType) CGColor];
        core.lineWidth = macos_overlay_support::ScaleOverlayMetric(size, 1.2, 160.0, 0.8, 2.4);
    } else {
        CGPathRef line = detail::CreateTrailLinePath(content.bounds, deltaX, deltaY, plan.trailType);
        core.path = line;
        CGPathRelease(line);
        core.fillColor = [NSColor clearColor].CGColor;
        core.strokeColor = [detail::TrailStrokeColor(plan.trailType) CGColor];
        core.lineCap = kCALineCapRound;
        core.lineJoin = kCALineJoinRound;
        core.lineWidth = macos_overlay_support::ScaleOverlayMetric(
            size,
            (plan.trailType == "meteor") ? 4.0 : 3.0,
            160.0,
            1.2,
            7.0);
    }

    core.opacity = static_cast<float>(macos_overlay_support::ClampOverlayOpacity(profile.baseOpacity));
}

void AddTrailGlowLayer(
    NSView* content,
    const TrailPulseRenderPlan& plan,
    const macos_effect_profile::TrailRenderProfile& profile) {
    if (!plan.glowMode) {
        return;
    }

    CAShapeLayer* glow = [CAShapeLayer layer];
    glow.frame = content.bounds;
    const CGFloat size = CGRectGetWidth(content.bounds);
    const CGFloat glowInset = macos_overlay_support::ScaleOverlayMetric(size, 18.0, 160.0, 8.0, 36.0);
    CGPathRef glowPath = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, glowInset, glowInset), nullptr);
    glow.path = glowPath;
    CGPathRelease(glowPath);
    glow.fillColor = [detail::TrailFillColor(plan.trailType) CGColor];
    glow.strokeColor = [NSColor clearColor].CGColor;
    glow.opacity = static_cast<float>(macos_overlay_support::ClampOverlayOpacity(profile.baseOpacity - 0.08));
    [content.layer addSublayer:glow];
}

void StartTrailPulseAnimation(
    CAShapeLayer* core,
    const TrailPulseRenderPlan& plan,
    const macos_effect_profile::TrailRenderProfile& profile) {
    CAAnimationGroup* group = macos_overlay_support::CreateScaleFadeAnimationGroup(
        0.65,
        1.0,
        static_cast<CGFloat>(profile.baseOpacity),
        plan.durationSec);
    [core addAnimation:group forKey:@"mfx_trail_pulse"];
}

#endif

} // namespace mousefx::macos_trail_pulse
