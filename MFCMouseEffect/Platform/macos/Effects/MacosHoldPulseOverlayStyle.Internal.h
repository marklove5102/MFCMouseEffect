#pragma once

#include "Platform/macos/Effects/MacosHoldPulseOverlayStyle.h"

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#ifdef __OBJC__
@class NSColor;
@class CAShapeLayer;
#else
struct objc_object;
using NSColor = objc_object;
using CAShapeLayer = objc_object;
#endif
#endif

namespace mousefx::macos_hold_pulse::detail {

#if defined(__APPLE__)
bool ConfigureSpecialHoldAccentLayer(CAShapeLayer* accent, CGRect bounds, HoldStyle holdStyle, NSColor* baseColor);
#endif

} // namespace mousefx::macos_hold_pulse::detail
