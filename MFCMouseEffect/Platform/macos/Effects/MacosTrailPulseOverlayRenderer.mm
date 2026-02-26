#include "pch.h"

#include "Platform/macos/Effects/MacosTrailPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosTrailPulseOverlayStyle.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosTrailPulseWindowRegistry.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#endif

namespace mousefx::macos_trail_pulse {

#if defined(__APPLE__)
namespace {

void ShowTrailPulseOverlayOnMain(
    const ScreenPoint& overlayPt,
    double deltaX,
    double deltaY,
    const std::string& effectType,
    const std::string& themeName,
    const macos_effect_profile::TrailRenderProfile& profile) {
    (void)themeName;
    const std::string trailType = detail::NormalizeTrailType(effectType);

    const CGFloat size = (trailType == "particle")
        ? static_cast<CGFloat>(profile.particleSizePx)
        : static_cast<CGFloat>(profile.normalSizePx);
    const NSRect frame = NSMakeRect(overlayPt.x - size * 0.5, overlayPt.y - size * 0.5, size, size);
    NSWindow* window = macos_overlay_support::CreateOverlayWindow(frame);
    if (window == nil) {
        return;
    }

    NSView* content = [window contentView];
    [content setWantsLayer:YES];

    CAShapeLayer* core = [CAShapeLayer layer];
    core.frame = content.bounds;

    if (trailType == "tubes") {
        CGPathRef outer = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, 9.0, 9.0), nullptr);
        core.path = outer;
        CGPathRelease(outer);
        core.fillColor = [NSColor clearColor].CGColor;
        core.strokeColor = [detail::TrailStrokeColor(trailType) CGColor];
        core.lineWidth = 3.2;
    } else if (trailType == "particle") {
        CGPathRef dot = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, 16.0, 16.0), nullptr);
        core.path = dot;
        CGPathRelease(dot);
        core.fillColor = [detail::TrailStrokeColor(trailType) CGColor];
        core.strokeColor = [detail::TrailStrokeColor(trailType) CGColor];
        core.lineWidth = 1.2;
    } else {
        CGPathRef line = detail::CreateTrailLinePath(content.bounds, deltaX, deltaY, trailType);
        core.path = line;
        CGPathRelease(line);
        core.fillColor = [NSColor clearColor].CGColor;
        core.strokeColor = [detail::TrailStrokeColor(trailType) CGColor];
        core.lineCap = kCALineCapRound;
        core.lineJoin = kCALineJoinRound;
        core.lineWidth = (trailType == "meteor") ? 4.0 : 3.0;
    }

    core.opacity = static_cast<float>(profile.baseOpacity);
    [content.layer addSublayer:core];

    if (trailType == "meteor" || trailType == "streamer") {
        CAShapeLayer* glow = [CAShapeLayer layer];
        glow.frame = content.bounds;
        CGPathRef glowPath = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, 18.0, 18.0), nullptr);
        glow.path = glowPath;
        CGPathRelease(glowPath);
        glow.fillColor = [detail::TrailFillColor(trailType) CGColor];
        glow.strokeColor = [NSColor clearColor].CGColor;
        glow.opacity = 0.85;
        [content.layer addSublayer:glow];
    }

    CABasicAnimation* scale = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
    scale.fromValue = @0.65;
    scale.toValue = @1.0;
    scale.duration = profile.durationSec;
    scale.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];

    CABasicAnimation* fade = [CABasicAnimation animationWithKeyPath:@"opacity"];
    fade.fromValue = @(profile.baseOpacity);
    fade.toValue = @0.0;
    fade.duration = profile.durationSec;
    fade.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];

    CAAnimationGroup* group = [CAAnimationGroup animation];
    group.animations = @[scale, fade];
    group.duration = profile.durationSec;
    group.fillMode = kCAFillModeForwards;
    group.removedOnCompletion = NO;
    [core addAnimation:group forKey:@"mfx_trail_pulse"];

    RegisterTrailPulseWindow(reinterpret_cast<void*>(window));
    [window orderFrontRegardless];

    dispatch_after(
        dispatch_time(
            DISPATCH_TIME_NOW,
            static_cast<int64_t>(static_cast<int>(profile.durationSec * 1000.0) + profile.closePaddingMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          if (!TakeTrailPulseWindow(reinterpret_cast<void*>(window))) {
              return;
          }
          [window orderOut:nil];
          [window release];
        });
}

} // namespace
#endif

void CloseAllTrailPulseWindows() {
#if !defined(__APPLE__)
    return;
#else
    macos_overlay_support::RunOnMainThreadSync(^{
      CloseAllTrailPulseWindowsNow();
    });
#endif
}

void ShowTrailPulseOverlay(
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
    const ScreenPoint ptCopy = overlayPt;
    const double dxCopy = deltaX;
    const double dyCopy = deltaY;
    const std::string typeCopy = effectType;
    const std::string themeCopy = themeName;
    const macos_effect_profile::TrailRenderProfile profileCopy = profile;
    macos_overlay_support::RunOnMainThreadAsync(^{
      ShowTrailPulseOverlayOnMain(ptCopy, dxCopy, dyCopy, typeCopy, themeCopy, profileCopy);
    });
#endif
}

void ShowTrailPulseOverlay(
    const ScreenPoint& overlayPt,
    double deltaX,
    double deltaY,
    const std::string& effectType,
    const std::string& themeName) {
    ShowTrailPulseOverlay(overlayPt, deltaX, deltaY, effectType, themeName, macos_effect_profile::DefaultTrailRenderProfile(effectType));
}

} // namespace mousefx::macos_trail_pulse
