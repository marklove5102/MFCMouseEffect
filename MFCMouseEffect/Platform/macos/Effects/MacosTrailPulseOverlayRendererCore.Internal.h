#pragma once

#include "MouseFx/Core/Effects/TrailEffectCompute.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace mousefx::macos_trail_pulse {

#if defined(__APPLE__)
struct TrailPulseRenderPlan {
    TrailEffectRenderCommand command{};
    CGFloat size = 0;
    CGRect frame = CGRectZero;
    CFTimeInterval durationSec = 0;
    int closeAfterMs = 0;
};

TrailPulseRenderPlan BuildTrailPulseRenderPlan(const TrailEffectRenderCommand& command);
#endif

} // namespace mousefx::macos_trail_pulse
