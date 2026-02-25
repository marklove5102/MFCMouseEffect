#pragma once

#include "MouseFx/Core/Protocol/InputTypes.h"

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace mousefx::macos_click_pulse {

#if defined(__APPLE__)
NSColor* ClickPulseStrokeColor(MouseButton button);
NSColor* ClickPulseFillColor(MouseButton button);
#endif

} // namespace mousefx::macos_click_pulse
