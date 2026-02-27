#pragma once

#include "Platform/macos/Effects/MacosEffectRenderProfile.h"

#include <string>

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#endif

namespace mousefx::macos_trail_pulse::detail {

#if defined(__APPLE__)
std::string NormalizeTrailType(const std::string& effectType);
NSColor* TrailStrokeColor(
    const std::string& trailType,
    const macos_effect_profile::TrailRenderProfile& profile);
NSColor* TrailFillColor(
    const std::string& trailType,
    const macos_effect_profile::TrailRenderProfile& profile);
CGPathRef CreateTrailLinePath(CGRect bounds, double deltaX, double deltaY, const std::string& trailType);
#endif

} // namespace mousefx::macos_trail_pulse::detail
