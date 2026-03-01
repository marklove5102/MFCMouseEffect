#pragma once

#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "Platform/macos/Effects/MacosHoldPulseOverlayStyle.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>

#if defined(__APPLE__)
#ifdef __OBJC__
@class NSWindow;
@class CAShapeLayer;
#else
struct objc_object;
using NSWindow = objc_object;
using CAShapeLayer = objc_object;
#endif
#endif

namespace mousefx::macos_hold_pulse::detail {

#if defined(__APPLE__)
struct HoldOverlayState {
    NSWindow* window = nullptr;
    CAShapeLayer* ring = nullptr;
    CAShapeLayer* accent = nullptr;
    macos_effect_profile::HoldRenderProfile profile{};
    HoldStyle style = HoldStyle::Charge;
    std::string effectType{};
    MouseButton button = MouseButton::Left;
};
HoldOverlayState& State();
#endif

} // namespace mousefx::macos_hold_pulse::detail
