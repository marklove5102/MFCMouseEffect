#include "pch.h"

#include "Platform/macos/Effects/MacosScrollPulseEffect.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#endif

#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_set>

namespace mousefx {

#if defined(__APPLE__)
namespace {

std::mutex& ScrollWindowMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_set<NSWindow*>& ScrollWindows() {
    static std::unordered_set<NSWindow*> windows;
    return windows;
}

void RegisterScrollWindow(NSWindow* window) {
    if (window == nil) {
        return;
    }
    std::lock_guard<std::mutex> lock(ScrollWindowMutex());
    ScrollWindows().insert(window);
}

bool TakeScrollWindow(NSWindow* window) {
    if (window == nil) {
        return false;
    }
    std::lock_guard<std::mutex> lock(ScrollWindowMutex());
    auto& windows = ScrollWindows();
    const auto it = windows.find(window);
    if (it == windows.end()) {
        return false;
    }
    windows.erase(it);
    return true;
}

void CloseAllScrollWindowsNow() {
    std::unordered_set<NSWindow*> windows;
    {
        std::lock_guard<std::mutex> lock(ScrollWindowMutex());
        windows.swap(ScrollWindows());
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

NSColor* ScrollStrokeColor(bool horizontal, int delta) {
    if (horizontal) {
        return (delta >= 0)
            ? [NSColor colorWithCalibratedRed:0.35 green:0.88 blue:0.95 alpha:0.96]
            : [NSColor colorWithCalibratedRed:0.62 green:0.80 blue:1 alpha:0.96];
    }
    return (delta >= 0)
        ? [NSColor colorWithCalibratedRed:0.42 green:0.92 blue:0.56 alpha:0.96]
        : [NSColor colorWithCalibratedRed:1.0 green:0.57 blue:0.34 alpha:0.96];
}

NSColor* ScrollFillColor(bool horizontal, int delta) {
    if (horizontal) {
        return (delta >= 0)
            ? [NSColor colorWithCalibratedRed:0.35 green:0.88 blue:0.95 alpha:0.24]
            : [NSColor colorWithCalibratedRed:0.62 green:0.80 blue:1 alpha:0.24];
    }
    return (delta >= 0)
        ? [NSColor colorWithCalibratedRed:0.42 green:0.92 blue:0.56 alpha:0.24]
        : [NSColor colorWithCalibratedRed:1.0 green:0.57 blue:0.34 alpha:0.24];
}

CGPathRef CreateDirectionArrowPath(CGRect bodyRect, bool horizontal, int delta) {
    const bool positive = (delta >= 0);
    const CGFloat size = 7.0;
    const CGFloat cx = horizontal
        ? (positive ? CGRectGetMaxX(bodyRect) - 9.0 : CGRectGetMinX(bodyRect) + 9.0)
        : CGRectGetMidX(bodyRect);
    const CGFloat cy = horizontal
        ? CGRectGetMidY(bodyRect)
        : (positive ? CGRectGetMaxY(bodyRect) - 9.0 : CGRectGetMinY(bodyRect) + 9.0);

    CGMutablePathRef path = CGPathCreateMutable();
    if (horizontal) {
        if (positive) {
            CGPathMoveToPoint(path, nullptr, cx + size, cy);
            CGPathAddLineToPoint(path, nullptr, cx - size * 0.8, cy + size * 0.8);
            CGPathAddLineToPoint(path, nullptr, cx - size * 0.8, cy - size * 0.8);
        } else {
            CGPathMoveToPoint(path, nullptr, cx - size, cy);
            CGPathAddLineToPoint(path, nullptr, cx + size * 0.8, cy + size * 0.8);
            CGPathAddLineToPoint(path, nullptr, cx + size * 0.8, cy - size * 0.8);
        }
    } else {
        if (positive) {
            CGPathMoveToPoint(path, nullptr, cx, cy + size);
            CGPathAddLineToPoint(path, nullptr, cx - size * 0.8, cy - size * 0.8);
            CGPathAddLineToPoint(path, nullptr, cx + size * 0.8, cy - size * 0.8);
        } else {
            CGPathMoveToPoint(path, nullptr, cx, cy - size);
            CGPathAddLineToPoint(path, nullptr, cx - size * 0.8, cy + size * 0.8);
            CGPathAddLineToPoint(path, nullptr, cx + size * 0.8, cy + size * 0.8);
        }
    }
    CGPathCloseSubpath(path);
    return path;
}

} // namespace
#endif

MacosScrollPulseEffect::MacosScrollPulseEffect(std::string themeName)
    : themeName_(std::move(themeName)) {
}

MacosScrollPulseEffect::~MacosScrollPulseEffect() {
    Shutdown();
}

bool MacosScrollPulseEffect::Initialize() {
    initialized_ = true;
    return true;
}

void MacosScrollPulseEffect::Shutdown() {
    initialized_ = false;
#if defined(__APPLE__)
    RunOnMainThreadSync(^{
      CloseAllScrollWindowsNow();
    });
#endif
}

void MacosScrollPulseEffect::OnScroll(const ScrollEvent& event) {
    if (!initialized_) {
        return;
    }
#if !defined(__APPLE__)
    (void)event;
    return;
#else
    if (event.delta == 0) {
        return;
    }

    const ScreenPoint pt = ScreenToOverlayPoint(event.pt);
    const bool horizontal = event.horizontal;
    const int delta = event.delta;
    const int strengthLevel = std::clamp(static_cast<int>(std::abs(delta) / 120), 1, 6);
    const std::string theme = themeName_;

    RunOnMainThreadAsync(^{
      (void)theme;
      const CGFloat size = horizontal ? 148.0 : 138.0;
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

      const CGFloat bodyThickness = 18.0;
      const CGFloat bodyLength = 56.0 + static_cast<CGFloat>(strengthLevel) * 8.0;
      const CGRect bodyRect = horizontal
          ? CGRectMake((size - bodyLength) * 0.5, (size - bodyThickness) * 0.5, bodyLength, bodyThickness)
          : CGRectMake((size - bodyThickness) * 0.5, (size - bodyLength) * 0.5, bodyThickness, bodyLength);

      CAShapeLayer* body = [CAShapeLayer layer];
      body.frame = content.bounds;
      CGPathRef bodyPath = CGPathCreateWithRoundedRect(bodyRect, bodyThickness * 0.5, bodyThickness * 0.5, nullptr);
      body.path = bodyPath;
      CGPathRelease(bodyPath);
      body.fillColor = [ScrollFillColor(horizontal, delta) CGColor];
      body.strokeColor = [ScrollStrokeColor(horizontal, delta) CGColor];
      body.lineWidth = 2.0;
      body.opacity = 0.96;
      [content.layer addSublayer:body];

      CAShapeLayer* arrow = [CAShapeLayer layer];
      arrow.frame = content.bounds;
      CGPathRef arrowPath = CreateDirectionArrowPath(bodyRect, horizontal, delta);
      arrow.path = arrowPath;
      CGPathRelease(arrowPath);
      arrow.fillColor = [ScrollStrokeColor(horizontal, delta) CGColor];
      arrow.opacity = 0.98;
      [content.layer addSublayer:arrow];

      const CFTimeInterval duration = 0.28 + static_cast<CFTimeInterval>(strengthLevel) * 0.018;
      CABasicAnimation* scale = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
      scale.fromValue = @0.72;
      scale.toValue = @1.04;
      scale.duration = duration;
      scale.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];

      CABasicAnimation* fade = [CABasicAnimation animationWithKeyPath:@"opacity"];
      fade.fromValue = @0.98;
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

      RegisterScrollWindow(window);
      [window orderFrontRegardless];

      const int closeAfterMs = static_cast<int>(duration * 1000.0) + 70;
      dispatch_after(
          dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(closeAfterMs) * NSEC_PER_MSEC),
          dispatch_get_main_queue(),
          ^{
            if (!TakeScrollWindow(window)) {
                return;
            }
            [window orderOut:nil];
            [window release];
          });
    });
#endif
}

} // namespace mousefx
