#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseOverlayRendererCore.Internal.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosClickPulseOverlayStyle.h"

#include <algorithm>

namespace mousefx::macos_click_pulse {

#if defined(__APPLE__)
void ConfigureClickPulseBaseLayer(
    CAShapeLayer* base,
    NSView* content,
    MouseButton button,
    const ClickPulseRenderPlan& plan,
    const macos_effect_profile::ClickRenderProfile& profile) {
    base.frame = content.bounds;
    CGPathRef ringPath = CGPathCreateWithEllipseInRect(
        CGRectMake(plan.inset, plan.inset, plan.size - plan.inset * 2.0, plan.size - plan.inset * 2.0),
        nullptr);
    base.path = ringPath;
    CGPathRelease(ringPath);
    base.fillColor = [ClickPulseFillColor(button) CGColor];
    base.strokeColor = [ClickPulseStrokeColor(button) CGColor];
    base.lineWidth = macos_overlay_support::ScaleOverlayMetric(
        plan.size,
        plan.textMode ? 2.1 : 2.4,
        160.0,
        1.2,
        4.8);
    base.opacity = static_cast<float>(macos_overlay_support::ResolveOverlayOpacity(profile.baseOpacity, 0.0, 0.0));
}

void AddClickPulseExtraLayers(
    NSView* content,
    MouseButton button,
    const ClickPulseRenderPlan& plan,
    const macos_effect_profile::ClickRenderProfile& profile) {
    if (plan.starMode) {
        CAShapeLayer* star = [CAShapeLayer layer];
        star.frame = content.bounds;
        const CGFloat starInset = macos_overlay_support::ScaleOverlayMetric(plan.size, 38.0, 160.0, 18.0, 74.0);
        const CGRect starBounds = CGRectInset(content.bounds, starInset, starInset);
        CGPathRef starPath = CreateClickPulseStarPath(starBounds, 5);
        star.path = starPath;
        CGPathRelease(starPath);
        star.fillColor = [ClickPulseStrokeColor(button) CGColor];
        star.strokeColor = [ClickPulseStrokeColor(button) CGColor];
        star.lineWidth = macos_overlay_support::ScaleOverlayMetric(plan.size, 1.0, 160.0, 0.8, 2.2);
        star.opacity = static_cast<float>(macos_overlay_support::ResolveOverlayOpacity(profile.baseOpacity, 0.03, 0.0));
        [content.layer addSublayer:star];
    }

    if (plan.textMode) {
        CATextLayer* text = [CATextLayer layer];
        const CGFloat textHeight = macos_overlay_support::ScaleOverlayMetric(plan.size, 36.0, 160.0, 24.0, 60.0);
        text.frame = CGRectMake(0.0, plan.size * 0.30, plan.size, textHeight);
        text.alignmentMode = kCAAlignmentCenter;
        text.foregroundColor = [ClickPulseStrokeColor(button) CGColor];
        text.contentsScale = std::max<CGFloat>(1.0, content.layer.contentsScale);
        const CGFloat fontSize = macos_overlay_support::ScaleOverlayMetric(plan.size, 24.0, 160.0, 14.0, 42.0);
        text.fontSize = fontSize;
        text.font = (__bridge CFTypeRef)[NSFont boldSystemFontOfSize:fontSize];
        switch (button) {
        case MouseButton::Right:
            text.string = @"RIGHT";
            break;
        case MouseButton::Middle:
            text.string = @"MIDDLE";
            break;
        case MouseButton::Left:
        default:
            text.string = @"LEFT";
            break;
        }
        text.opacity = static_cast<float>(macos_overlay_support::ResolveOverlayOpacity(profile.baseOpacity, 0.03, 0.0));
        [content.layer addSublayer:text];
    }
}

void StartClickPulseAnimation(
    CAShapeLayer* base,
    const ClickPulseRenderPlan& plan,
    const macos_effect_profile::ClickRenderProfile& profile) {
    CAAnimationGroup* group = macos_overlay_support::CreateScaleFadeAnimationGroup(
        plan.textMode ? 0.75 : 0.15,
        1.0,
        static_cast<CGFloat>(profile.baseOpacity),
        plan.animationDuration);
    [base addAnimation:group forKey:@"mfx_click_pulse"];
}

#endif

} // namespace mousefx::macos_click_pulse
