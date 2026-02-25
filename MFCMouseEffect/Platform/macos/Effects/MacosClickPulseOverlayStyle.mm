#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseOverlayStyle.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace mousefx::macos_click_pulse {

#if defined(__APPLE__)
NSColor* ClickPulseStrokeColor(MouseButton button) {
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

NSColor* ClickPulseFillColor(MouseButton button) {
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
#endif

} // namespace mousefx::macos_click_pulse
