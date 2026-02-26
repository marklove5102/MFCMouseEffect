#include "pch.h"

#include "Platform/macos/Effects/MacosHoverPulseOverlayStyle.h"
#include "MouseFx/Utils/StringUtils.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace mousefx::macos_hover_pulse {

#if defined(__APPLE__)
namespace {

bool ContainsHoverToken(const std::string& value, const char* token) {
    return value.find(token) != std::string::npos;
}

} // namespace

std::string NormalizeHoverType(const std::string& effectType) {
    const std::string value = ToLowerAscii(effectType);
    if (value.empty() || value == "none") {
        return "glow";
    }
    if (ContainsHoverToken(value, "tube") ||
        ContainsHoverToken(value, "suspension") ||
        ContainsHoverToken(value, "helix")) {
        return "tubes";
    }
    if (ContainsHoverToken(value, "glow") || ContainsHoverToken(value, "breath")) {
        return "glow";
    }
    return "glow";
}

NSColor* HoverGlowFillColor() {
    return [NSColor colorWithCalibratedRed:0.25 green:0.70 blue:1.0 alpha:0.10];
}

NSColor* HoverGlowStrokeColor() {
    return [NSColor colorWithCalibratedRed:0.25 green:0.70 blue:1.0 alpha:0.95];
}

NSColor* HoverTubesStrokeColor() {
    return [NSColor colorWithCalibratedRed:0.47 green:0.90 blue:0.63 alpha:0.95];
}
#endif

} // namespace mousefx::macos_hover_pulse
