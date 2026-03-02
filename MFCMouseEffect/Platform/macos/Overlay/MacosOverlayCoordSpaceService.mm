#include "pch.h"

#include "Platform/macos/Overlay/MacosOverlayCoordSpaceService.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <dispatch/dispatch.h>
#endif

#include <cmath>

namespace mousefx {

namespace {

ScreenPoint ToScreenPoint(double x, double y) {
    ScreenPoint out{};
    out.x = static_cast<int32_t>(std::lround(x));
    out.y = static_cast<int32_t>(std::lround(y));
    return out;
}

#if defined(__APPLE__)
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

bool TryConvertQuartzToCocoa(const ScreenPoint& input, ScreenPoint* output) {
    if (!output) {
        return false;
    }

    __block bool converted = false;
    __block ScreenPoint convertedPoint = input;

    RunOnMainThreadSync(^{
      NSArray<NSScreen*>* screens = [NSScreen screens];
      if (screens == nil || [screens count] == 0) {
          return;
      }

      const CGPoint quartzPt = CGPointMake(static_cast<CGFloat>(input.x), static_cast<CGFloat>(input.y));

      NSScreen* matchedScreen = nil;
      CGRect matchedBounds = CGRectZero;
      for (NSScreen* screen in screens) {
          NSDictionary* desc = [screen deviceDescription];
          NSNumber* screenNumber = desc[@"NSScreenNumber"];
          if (screenNumber == nil) {
              continue;
          }
          const CGDirectDisplayID displayId = static_cast<CGDirectDisplayID>([screenNumber unsignedIntValue]);
          const CGRect bounds = CGDisplayBounds(displayId);
          if (CGRectContainsPoint(bounds, quartzPt)) {
              matchedScreen = screen;
              matchedBounds = bounds;
              break;
          }
      }

      if (matchedScreen == nil) {
          matchedScreen = [NSScreen mainScreen];
          if (matchedScreen == nil) {
              matchedScreen = [screens objectAtIndex:0];
          }
          NSDictionary* desc = [matchedScreen deviceDescription];
          NSNumber* screenNumber = desc[@"NSScreenNumber"];
          if (screenNumber != nil) {
              const CGDirectDisplayID displayId = static_cast<CGDirectDisplayID>([screenNumber unsignedIntValue]);
              matchedBounds = CGDisplayBounds(displayId);
          }
      }

      if (matchedScreen == nil) {
          return;
      }

      const NSRect frame = [matchedScreen frame];
      if (matchedBounds.size.width <= 0.0 || matchedBounds.size.height <= 0.0 ||
          frame.size.width <= 0.0 || frame.size.height <= 0.0) {
          convertedPoint = input;
          converted = true;
          return;
      }

      const double scaleX = static_cast<double>(frame.size.width) / static_cast<double>(matchedBounds.size.width);
      const double scaleY = static_cast<double>(frame.size.height) / static_cast<double>(matchedBounds.size.height);
      const double localX = (static_cast<double>(input.x) - static_cast<double>(matchedBounds.origin.x)) * scaleX;
      const double localY = (static_cast<double>(input.y) - static_cast<double>(matchedBounds.origin.y)) * scaleY;

      const double cocoaX = static_cast<double>(frame.origin.x) + localX;
      const double cocoaY = static_cast<double>(frame.origin.y) + static_cast<double>(frame.size.height) - localY;
      convertedPoint = ToScreenPoint(cocoaX, cocoaY);
      converted = true;
    });

    if (!converted) {
        return false;
    }

    *output = convertedPoint;
    return true;
}
#endif

} // namespace

void MacosOverlayCoordSpaceService::SetOverlayWindowHandle(uintptr_t hwndValue) {
    overlayWindowHandle_.store(hwndValue, std::memory_order_release);
}

void MacosOverlayCoordSpaceService::ClearOverlayWindowHandle() {
    overlayWindowHandle_.store(0, std::memory_order_release);
}

void MacosOverlayCoordSpaceService::SetOverlayOriginOverride(int x, int y) {
    overlayOriginX_.store(x, std::memory_order_relaxed);
    overlayOriginY_.store(y, std::memory_order_relaxed);
    overlayOriginOverrideEnabled_.store(true, std::memory_order_release);
}

void MacosOverlayCoordSpaceService::ClearOverlayOriginOverride() {
    overlayOriginOverrideEnabled_.store(false, std::memory_order_release);
}

ScreenPoint MacosOverlayCoordSpaceService::GetOverlayOrigin() const {
    if (overlayOriginOverrideEnabled_.load(std::memory_order_acquire)) {
        ScreenPoint pt{};
        pt.x = overlayOriginX_.load(std::memory_order_relaxed);
        pt.y = overlayOriginY_.load(std::memory_order_relaxed);
        return pt;
    }
    return {};
}

ScreenPoint MacosOverlayCoordSpaceService::ScreenToOverlayPoint(const ScreenPoint& screenPt) const {
    ScreenPoint converted = screenPt;
#if defined(__APPLE__)
    ScreenPoint cocoaPt{};
    if (TryConvertQuartzToCocoa(screenPt, &cocoaPt)) {
        converted = cocoaPt;
    }
#endif
    const ScreenPoint origin = GetOverlayOrigin();
    converted.x -= origin.x;
    converted.y -= origin.y;
    return converted;
}

} // namespace mousefx
