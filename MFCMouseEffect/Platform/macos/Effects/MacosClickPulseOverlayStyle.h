#pragma once

#include "MouseFx/Core/Protocol/InputTypes.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"

#include <string>

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace mousefx::macos_click_pulse {

#if defined(__APPLE__)
std::string NormalizeClickType(const std::string& effectType);
NSColor* ClickPulseStrokeColor(
    MouseButton button,
    const macos_effect_profile::ClickRenderProfile& profile);
NSColor* ClickPulseFillColor(
    MouseButton button,
    const macos_effect_profile::ClickRenderProfile& profile);
CGPathRef CreateClickPulseStarPath(CGRect bounds, int points);
#endif

} // namespace mousefx::macos_click_pulse
