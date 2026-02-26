#pragma once

#include <string>

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace mousefx::macos_hover_pulse {

#if defined(__APPLE__)
std::string NormalizeHoverType(const std::string& effectType);
NSColor* HoverGlowFillColor();
NSColor* HoverGlowStrokeColor();
NSColor* HoverTubesStrokeColor();
#endif

} // namespace mousefx::macos_hover_pulse
