#include "pch.h"

#include "Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.h"

#include "Platform/macos/Effects/MacosHoverPulseOverlayStyle.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#endif

#include <algorithm>

namespace mousefx::macos_hover_pulse {

#if defined(__APPLE__)
namespace {

NSWindow*& ActiveHoverWindow() {
    static NSWindow* window = nil;
    return window;
}

} // namespace
#endif

void CloseHoverPulseOverlayOnMain() {
#if !defined(__APPLE__)
    return;
#else
    NSWindow* window = ActiveHoverWindow();
    ActiveHoverWindow() = nil;
    if (window == nil) {
        return;
    }
    [window orderOut:nil];
    [window release];
#endif
}

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
    (void)themeName;
    CloseHoverPulseOverlayOnMain();

    const std::string hoverType = NormalizeHoverType(effectType);
    const CGFloat size = static_cast<CGFloat>(profile.sizePx);
    const NSRect frame = NSMakeRect(overlayPt.x - size * 0.5, overlayPt.y - size * 0.5, size, size);

    NSWindow* window = macos_overlay_support::CreateOverlayWindow(frame);
    if (window == nil) {
        return;
    }

    NSView* content = [window contentView];
    [content setWantsLayer:YES];

    CAShapeLayer* ring = [CAShapeLayer layer];
    ring.frame = content.bounds;
    CGPathRef ringPath = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, 20.0, 20.0), nullptr);
    ring.path = ringPath;
    CGPathRelease(ringPath);
    ring.fillColor = HoverGlowFillColor().CGColor;
    ring.strokeColor = HoverGlowStrokeColor().CGColor;
    ring.lineWidth = 2.0;
    ring.opacity = static_cast<float>(profile.baseOpacity);
    [content.layer addSublayer:ring];

    CABasicAnimation* breathe = [CABasicAnimation animationWithKeyPath:@"opacity"];
    breathe.fromValue = @0.25;
    breathe.toValue = @(std::min(1.0, profile.baseOpacity + 0.05));
    breathe.duration = profile.breatheDurationSec;
    breathe.autoreverses = YES;
    breathe.repeatCount = HUGE_VALF;
    [ring addAnimation:breathe forKey:@"mfx_hover_breathe"];

    if (hoverType == "tubes") {
        CAShapeLayer* ring2 = [CAShapeLayer layer];
        ring2.frame = content.bounds;
        CGPathRef ring2Path = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, 34.0, 34.0), nullptr);
        ring2.path = ring2Path;
        CGPathRelease(ring2Path);
        ring2.fillColor = [NSColor clearColor].CGColor;
        ring2.strokeColor = HoverTubesStrokeColor().CGColor;
        ring2.lineWidth = 1.8;
        ring2.opacity = static_cast<float>(std::max(0.1, profile.baseOpacity - 0.05));
        [content.layer addSublayer:ring2];

        CABasicAnimation* spin = [CABasicAnimation animationWithKeyPath:@"transform.rotation"];
        spin.fromValue = @0.0;
        spin.toValue = @(M_PI * 2.0);
        spin.duration = profile.spinDurationSec;
        spin.repeatCount = HUGE_VALF;
        [ring2 addAnimation:spin forKey:@"mfx_hover_spin"];
    }

    [window orderFrontRegardless];
    ActiveHoverWindow() = window;
#endif
}

size_t GetActiveHoverPulseWindowCountOnMain() {
#if !defined(__APPLE__)
    return 0;
#else
    return (ActiveHoverWindow() == nil) ? 0 : 1;
#endif
}

} // namespace mousefx::macos_hover_pulse
