#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseOverlayRenderer.h"
#include "Platform/macos/Effects/MacosClickPulseOverlayStyle.h"
#include "Platform/macos/Effects/MacosClickPulseWindowRegistry.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#endif

namespace mousefx::macos_click_pulse {

#if defined(__APPLE__)
namespace {

void RunOnMainThreadSync(dispatch_block_t block) {
    if (!block) {
        return;
    }
    if ([NSThread isMainThread]) {
        block();
        return;
    }
    dispatch_sync(dispatch_get_main_queue(), block);
}

void RunOnMainThreadAsync(dispatch_block_t block) {
    if (!block) {
        return;
    }
    dispatch_async(dispatch_get_main_queue(), block);
}

void ShowClickPulseOverlayOnMain(const ScreenPoint& overlayPt, MouseButton button, const std::string& themeName) {
    const std::string theme = themeName;
    (void)theme;

    const CGFloat size = 138.0;
    const NSRect frame = NSMakeRect(overlayPt.x - size * 0.5, overlayPt.y - size * 0.5, size, size);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskBorderless
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    if (window == nil) {
        return;
    }
    [window setOpaque:NO];
    [window setBackgroundColor:[NSColor clearColor]];
    [window setHasShadow:NO];
    [window setIgnoresMouseEvents:YES];
    [window setLevel:NSStatusWindowLevel];
    [window setCollectionBehavior:(NSWindowCollectionBehaviorCanJoinAllSpaces |
                                   NSWindowCollectionBehaviorTransient)];

    NSView* content = [window contentView];
    [content setWantsLayer:YES];

    CAShapeLayer* ring = [CAShapeLayer layer];
    const CGFloat inset = 18.0;
    ring.frame = content.bounds;
    CGPathRef ringPath = CGPathCreateWithEllipseInRect(
        CGRectMake(inset, inset, size - inset * 2.0, size - inset * 2.0),
        nullptr);
    ring.path = ringPath;
    CGPathRelease(ringPath);
    ring.fillColor = [ClickPulseFillColor(button) CGColor];
    ring.strokeColor = [ClickPulseStrokeColor(button) CGColor];
    ring.lineWidth = 2.4;
    ring.opacity = 0.95;
    [content.layer addSublayer:ring];

    CABasicAnimation* scale = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
    scale.fromValue = @0.15;
    scale.toValue = @1.0;
    scale.duration = 0.32;
    scale.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];

    CABasicAnimation* fade = [CABasicAnimation animationWithKeyPath:@"opacity"];
    fade.fromValue = @0.95;
    fade.toValue = @0.0;
    fade.duration = 0.32;
    fade.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];

    CAAnimationGroup* group = [CAAnimationGroup animation];
    group.animations = @[scale, fade];
    group.duration = 0.32;
    group.fillMode = kCAFillModeForwards;
    group.removedOnCompletion = NO;
    [ring addAnimation:group forKey:@"mfx_click_pulse"];

    RegisterClickPulseWindow(reinterpret_cast<void*>(window));
    [window orderFrontRegardless];

    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(360) * NSEC_PER_MSEC),
        dispatch_get_main_queue(),
        ^{
          if (!TakeClickPulseWindow(reinterpret_cast<void*>(window))) {
              return;
          }
          [window orderOut:nil];
          [window release];
        });
}

} // namespace
#endif

void CloseAllClickPulseWindows() {
#if !defined(__APPLE__)
    return;
#else
    RunOnMainThreadSync(^{
      CloseAllClickPulseWindowsNow();
    });
#endif
}

void ShowClickPulseOverlay(const ScreenPoint& overlayPt, MouseButton button, const std::string& themeName) {
#if !defined(__APPLE__)
    (void)overlayPt;
    (void)button;
    (void)themeName;
    return;
#else
    const ScreenPoint ptCopy = overlayPt;
    const MouseButton buttonCopy = button;
    const std::string themeCopy = themeName;
    RunOnMainThreadAsync(^{
      ShowClickPulseOverlayOnMain(ptCopy, buttonCopy, themeCopy);
    });
#endif
}

} // namespace mousefx::macos_click_pulse
