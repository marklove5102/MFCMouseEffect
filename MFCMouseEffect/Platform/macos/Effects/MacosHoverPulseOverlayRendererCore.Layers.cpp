#include "pch.h"

#include "Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.h"
#include "Platform/macos/Effects/MacosHoverPulseOverlayRendererCore.Internal.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"

namespace mousefx::macos_hover_pulse {

#if defined(__APPLE__)
namespace {

NSWindow*& ActiveHoverWindow() {
    static NSWindow* window = nil;
    return window;
}

NSColor* ArgbToNsColor(uint32_t argb) {
    const CGFloat alpha = static_cast<CGFloat>((argb >> 24) & 0xFFu) / 255.0;
    const CGFloat red = static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0;
    const CGFloat green = static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0;
    const CGFloat blue = static_cast<CGFloat>(argb & 0xFFu) / 255.0;
    return [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:alpha];
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
    macos_overlay_support::ReleaseOverlayWindow(reinterpret_cast<void*>(window));
#endif
}

void ShowHoverPulseOverlayOnMain(
    const HoverEffectRenderCommand& command,
    const std::string& themeName) {
#if !defined(__APPLE__)
    (void)command;
    (void)themeName;
    return;
#else
    (void)themeName;
    CloseHoverPulseOverlayOnMain();

    const HoverPulseRenderPlan plan = BuildHoverPulseRenderPlan(command);
    NSWindow* window = macos_overlay_support::CreateOverlayWindow(plan.frame);
    if (window == nil) {
        return;
    }

    NSView* content = [window contentView];
    macos_overlay_support::ApplyOverlayContentScale(content, command.overlayPoint);

    CAShapeLayer* ring = [CAShapeLayer layer];
    ConfigureHoverRingLayer(ring, content, plan);
    [content.layer addSublayer:ring];

    CABasicAnimation* breathe = [CABasicAnimation animationWithKeyPath:@"opacity"];
    breathe.fromValue = @0.25;
    breathe.toValue = @(macos_overlay_support::ResolveOverlayOpacity(command.baseOpacity, 0.05, 0.0));
    breathe.duration = plan.breatheDurationSec;
    breathe.autoreverses = YES;
    breathe.repeatCount = HUGE_VALF;
    [ring addAnimation:breathe forKey:@"mfx_hover_breathe"];

    AddHoverExtraLayersAndAnimations(content, plan);

    macos_overlay_support::ShowOverlayWindow(reinterpret_cast<void*>(window));
    ActiveHoverWindow() = window;
#endif
}

void AddHoverExtraLayersAndAnimations(
    NSView* content,
    const HoverPulseRenderPlan& plan) {
#if !defined(__APPLE__)
    (void)content;
    (void)plan;
    return;
#else
    if (!plan.command.tubesMode) {
        return;
    }

    CAShapeLayer* ring2 = [CAShapeLayer layer];
    ring2.frame = content.bounds;
    const CGFloat size = CGRectGetWidth(content.bounds);
    const CGFloat ring2Inset = macos_overlay_support::ScaleOverlayMetric(size, 34.0, 160.0, 18.0, 68.0);
    CGPathRef ring2Path = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, ring2Inset, ring2Inset), nullptr);
    ring2.path = ring2Path;
    CGPathRelease(ring2Path);
    ring2.fillColor = [NSColor clearColor].CGColor;
    ring2.strokeColor = [ArgbToNsColor(plan.command.tubesStrokeArgb) CGColor];
    ring2.lineWidth = macos_overlay_support::ScaleOverlayMetric(size, 1.8, 160.0, 0.9, 3.8);
    ring2.opacity = static_cast<float>(
        macos_overlay_support::ResolveOverlayOpacity(plan.command.baseOpacity, -0.05, 0.1));
    [content.layer addSublayer:ring2];

    CABasicAnimation* spin = [CABasicAnimation animationWithKeyPath:@"transform.rotation"];
    spin.fromValue = @0.0;
    spin.toValue = @(M_PI * 2.0);
    spin.duration = plan.tubesSpinDurationSec;
    spin.repeatCount = HUGE_VALF;
    [ring2 addAnimation:spin forKey:@"mfx_hover_spin"];
#endif
}

void ConfigureHoverRingLayer(
    CAShapeLayer* ring,
    NSView* content,
    const HoverPulseRenderPlan& plan) {
#if !defined(__APPLE__)
    (void)ring;
    (void)content;
    (void)plan;
    return;
#else
    ring.frame = content.bounds;
    const CGFloat ringInset = macos_overlay_support::ScaleOverlayMetric(plan.size, 20.0, 160.0, 10.0, 40.0);
    CGPathRef ringPath = CGPathCreateWithEllipseInRect(CGRectInset(content.bounds, ringInset, ringInset), nullptr);
    ring.path = ringPath;
    CGPathRelease(ringPath);
    ring.fillColor = [ArgbToNsColor(plan.command.glowFillArgb) CGColor];
    ring.strokeColor = [ArgbToNsColor(plan.command.glowStrokeArgb) CGColor];
    ring.lineWidth = macos_overlay_support::ScaleOverlayMetric(plan.size, 2.0, 160.0, 1.0, 4.2);
    ring.opacity = static_cast<float>(
        macos_overlay_support::ResolveOverlayOpacity(plan.command.baseOpacity, 0.0, 0.0));
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
