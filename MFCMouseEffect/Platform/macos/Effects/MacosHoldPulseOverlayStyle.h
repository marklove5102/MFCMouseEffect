#pragma once

#include <string>

#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

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
enum class HoldStyle {
    Charge,
    Lightning,
    Hex,
    TechRing,
    Hologram,
    Neon,
    QuantumHalo,
    FluxField,
};

std::string NormalizeHoldType(const std::string& effectType);
HoldStyle ResolveHoldStyle(const std::string& holdType);
NSColor* HoldBaseColor(
    MouseButton button,
    HoldStyle style,
    const macos_effect_profile::HoldRenderProfile& profile);
void ConfigureHoldAccentLayer(CAShapeLayer* accent, CGRect bounds, HoldStyle holdStyle, NSColor* baseColor);
#endif

} // namespace mousefx::macos_hold_pulse::detail
