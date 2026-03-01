#pragma once

#include "MouseFx/Core/Effects/TrailEffectCompute.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#ifdef __OBJC__
@class CAShapeLayer;
@class NSView;
#else
struct objc_object;
using CAShapeLayer = objc_object;
using NSView = objc_object;
#endif
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

void ConfigureTrailCoreLayer(
    CAShapeLayer* core,
    NSView* content,
    const TrailPulseRenderPlan& plan,
    double deltaX,
    double deltaY);

void AddTrailGlowLayer(NSView* content, const TrailPulseRenderPlan& plan);
void StartTrailPulseAnimation(CAShapeLayer* core, const TrailPulseRenderPlan& plan);
#endif

} // namespace mousefx::macos_trail_pulse
