#include "pch.h"

#include "Platform/macos/Effects/MacosScrollPulseOverlayRendererCore.h"

#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "Platform/macos/Effects/MacosScrollPulseOverlayRendererSupport.h"
#include "Platform/macos/Effects/MacosScrollPulseOverlayStyle.h"
#include "Platform/macos/Effects/MacosScrollPulseWindowRegistry.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#endif

#include <cmath>

namespace mousefx::macos_scroll_pulse {

void ShowScrollPulseOverlayOnMain(
    const ScreenPoint& overlayPt,
    bool horizontal,
    int delta,
    const std::string& effectType,
    const std::string& themeName,
    const macos_effect_profile::ScrollRenderProfile& profile) {
#if !defined(__APPLE__)
    (void)overlayPt;
    (void)horizontal;
    (void)delta;
    (void)effectType;
    (void)themeName;
    (void)profile;
    return;
#else
    if (delta == 0) {
        return;
    }
    (void)themeName;

    const std::string normalizedType = NormalizeScrollType(effectType);
    const bool helixMode = (normalizedType == "helix");
    const bool twinkleMode = (normalizedType == "twinkle");

    const int strengthLevel = support::ResolveStrengthLevel(delta);

    const CGFloat size = horizontal ? static_cast<CGFloat>(profile.horizontalSizePx) : static_cast<CGFloat>(profile.verticalSizePx);
    const NSRect frame = NSMakeRect(overlayPt.x - size * 0.5, overlayPt.y - size * 0.5, size, size);
    NSWindow* window = macos_overlay_support::CreateOverlayWindow(frame);
    if (window == nil) {
        return;
    }

    NSView* content = [window contentView];
    [content setWantsLayer:YES];

    const CGRect bodyRect = support::BuildBodyRect(size, horizontal, strengthLevel);
    CAShapeLayer* body = support::CreateBodyLayer(content.bounds, bodyRect, horizontal, delta, profile.baseOpacity);
    [content.layer addSublayer:body];

    CAShapeLayer* arrow = support::CreateArrowLayer(content.bounds, bodyRect, horizontal, delta, profile.baseOpacity);
    [content.layer addSublayer:arrow];

    if (helixMode) {
        CAShapeLayer* helix = [CAShapeLayer layer];
        helix.frame = content.bounds;
        const CGRect helixRect = CGRectInset(bodyRect, -9.0, -9.0);
        CGPathRef helixPath = CGPathCreateWithEllipseInRect(helixRect, nullptr);
        helix.path = helixPath;
        CGPathRelease(helixPath);
        helix.fillColor = [NSColor clearColor].CGColor;
        helix.strokeColor = [ScrollPulseStrokeColor(horizontal, -delta) CGColor];
        helix.lineWidth = 1.6;
        helix.opacity = 0.82;
        [content.layer addSublayer:helix];

        CABasicAnimation* spin = [CABasicAnimation animationWithKeyPath:@"transform.rotation"];
        spin.fromValue = @0.0;
        spin.toValue = @(M_PI * 1.5);
        spin.duration = 0.45;
        spin.repeatCount = 1;
        [helix addAnimation:spin forKey:@"mfx_scroll_helix_spin"];
    }

    if (twinkleMode) {
        CAShapeLayer* twinkle = [CAShapeLayer layer];
        twinkle.frame = content.bounds;
        const CGRect twinkleRect = CGRectInset(bodyRect, -20.0, -20.0);
        CGPathRef twinklePath = CGPathCreateWithEllipseInRect(twinkleRect, nullptr);
        twinkle.path = twinklePath;
        CGPathRelease(twinklePath);
        twinkle.fillColor = [NSColor clearColor].CGColor;
        twinkle.strokeColor = [ScrollPulseStrokeColor(horizontal, delta) CGColor];
        twinkle.lineWidth = 1.0;
        twinkle.opacity = 0.55;
        [content.layer addSublayer:twinkle];
    }

    const CFTimeInterval duration = support::BuildPulseDuration(profile, strengthLevel);
    CABasicAnimation* scale = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
    scale.fromValue = @0.72;
    scale.toValue = @1.04;
    scale.duration = duration;
    scale.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];

    CABasicAnimation* fade = [CABasicAnimation animationWithKeyPath:@"opacity"];
    fade.fromValue = @(std::min(1.0, profile.baseOpacity + 0.02));
    fade.toValue = @0.0;
    fade.duration = duration;
    fade.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];

    CAAnimationGroup* group = [CAAnimationGroup animation];
    group.animations = @[scale, fade];
    group.duration = duration;
    group.fillMode = kCAFillModeForwards;
    group.removedOnCompletion = NO;
    [body addAnimation:group forKey:@"mfx_scroll_body_pulse"];
    [arrow addAnimation:group forKey:@"mfx_scroll_arrow_pulse"];

    RegisterScrollPulseWindow(reinterpret_cast<void*>(window));
    [window orderFrontRegardless];

    const int closeAfterMs = support::BuildCloseAfterMs(profile, duration);
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(closeAfterMs) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          if (!TakeScrollPulseWindow(reinterpret_cast<void*>(window))) {
              return;
          }
          [window orderOut:nil];
          [window release];
        });
#endif
}

} // namespace mousefx::macos_scroll_pulse
