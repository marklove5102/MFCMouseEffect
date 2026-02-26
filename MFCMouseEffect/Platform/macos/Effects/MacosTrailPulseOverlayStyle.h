#pragma once

#include <string>

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#endif

namespace mousefx::macos_trail_pulse::detail {

#if defined(__APPLE__)
std::string NormalizeTrailType(const std::string& effectType);
NSColor* TrailStrokeColor(const std::string& trailType);
NSColor* TrailFillColor(const std::string& trailType);
CGPathRef CreateTrailLinePath(CGRect bounds, double deltaX, double deltaY, const std::string& trailType);
#endif

} // namespace mousefx::macos_trail_pulse::detail
