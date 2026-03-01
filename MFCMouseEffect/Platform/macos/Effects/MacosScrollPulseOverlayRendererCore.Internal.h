#pragma once

#include "MouseFx/Core/Effects/ScrollEffectCompute.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

#include <cstdint>
#include <string>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace mousefx::macos_scroll_pulse {

#if defined(__APPLE__)
struct ScrollPulseRenderPlan {
    ScrollEffectRenderCommand command{};
    CGFloat size = 0;
    CGRect bodyRect = CGRectZero;
    CGRect frame = CGRectZero;
    CFTimeInterval duration = 0;
    int closeAfterMs = 0;
};

ScrollPulseRenderPlan BuildScrollPulseRenderPlan(const ScrollEffectRenderCommand& command);
#endif

} // namespace mousefx::macos_scroll_pulse
