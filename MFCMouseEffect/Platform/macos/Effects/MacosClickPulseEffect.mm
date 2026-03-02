#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseEffect.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#endif

#include <mutex>
#include <unordered_set>

namespace mousefx {

#if defined(__APPLE__)
namespace {

std::mutex& PulseWindowMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_set<NSWindow*>& PulseWindows() {
    static std::unordered_set<NSWindow*> windows;
    return windows;
}

void RegisterPulseWindow(NSWindow* window) {
    if (window == nil) {
        return;
    }
    std::lock_guard<std::mutex> lock(PulseWindowMutex());
    PulseWindows().insert(window);
}

bool TakePulseWindow(NSWindow* window) {
    if (window == nil) {
        return false;
    }
    std::lock_guard<std::mutex> lock(PulseWindowMutex());
    auto& windows = PulseWindows();
    const auto it = windows.find(window);
    if (it == windows.end()) {
        return false;
    }
    windows.erase(it);
    return true;
}

void CloseAllPulseWindowsNow() {
    std::unordered_set<NSWindow*> windows;
    {
        std::lock_guard<std::mutex> lock(PulseWindowMutex());
        windows.swap(PulseWindows());
    }
    for (NSWindow* window : windows) {
        if (window == nil) {
            continue;
        }
        [window orderOut:nil];
        [window release];
    }
}

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

NSColor* PulseStrokeColorForButton(MouseButton button) {
    switch (button) {
    case MouseButton::Left:
        return [NSColor colorWithCalibratedRed:0.22 green:0.70 blue:1 alpha:0.95];
    case MouseButton::Right:
        return [NSColor colorWithCalibratedRed:1.0 green:0.63 blue:0.22 alpha:0.95];
    case MouseButton::Middle:
        return [NSColor colorWithCalibratedRed:0.44 green:0.90 blue:0.57 alpha:0.95];
    default:
        return [NSColor colorWithCalibratedWhite:0.95 alpha:0.9];
    }
}

NSColor* PulseFillColorForButton(MouseButton button) {
    switch (button) {
    case MouseButton::Left:
        return [NSColor colorWithCalibratedRed:0.22 green:0.70 blue:1 alpha:0.22];
    case MouseButton::Right:
        return [NSColor colorWithCalibratedRed:1.0 green:0.63 blue:0.22 alpha:0.22];
    case MouseButton::Middle:
        return [NSColor colorWithCalibratedRed:0.44 green:0.90 blue:0.57 alpha:0.22];
    default:
        return [NSColor colorWithCalibratedWhite:0.95 alpha:0.18];
    }
}

} // namespace
#endif

MacosClickPulseEffect::MacosClickPulseEffect(std::string themeName)
    : themeName_(std::move(themeName)) {
}

MacosClickPulseEffect::~MacosClickPulseEffect() {
    Shutdown();
}

bool MacosClickPulseEffect::Initialize() {
    initialized_ = true;
    return true;
}

void MacosClickPulseEffect::Shutdown() {
    initialized_ = false;
#if defined(__APPLE__)
    RunOnMainThreadSync(^{
      CloseAllPulseWindowsNow();
    });
#endif
}

void MacosClickPulseEffect::OnClick(const ClickEvent& event) {
    if (!initialized_) {
        return;
    }
#if !defined(__APPLE__)
    (void)event;
    return;
#else
    const ScreenPoint pt = ScreenToOverlayPoint(event.pt);
    const MouseButton button = event.button;
    const std::string theme = themeName_;
    RunOnMainThreadAsync(^{
      (void)theme;
      const CGFloat size = 138.0;
      const NSRect frame = NSMakeRect(pt.x - size * 0.5, pt.y - size * 0.5, size, size);
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
      ring.fillColor = [PulseFillColorForButton(button) CGColor];
      ring.strokeColor = [PulseStrokeColorForButton(button) CGColor];
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

      RegisterPulseWindow(window);
      [window orderFrontRegardless];

      dispatch_after(
          dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(360) * NSEC_PER_MSEC),
          dispatch_get_main_queue(),
          ^{
            if (!TakePulseWindow(window)) {
                return;
            }
            [window orderOut:nil];
            [window release];
          });
    });
#endif
}

} // namespace mousefx
