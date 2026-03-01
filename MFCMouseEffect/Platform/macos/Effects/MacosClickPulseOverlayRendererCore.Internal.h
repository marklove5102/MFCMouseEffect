#pragma once

#include "MouseFx/Core/Effects/ClickEffectCompute.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

#include <cstdint>
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

namespace mousefx::macos_click_pulse {

#if defined(__APPLE__)
struct ClickPulseRenderPlan {
    ClickEffectRenderCommand command{};
    CGFloat size = 0;
    CGFloat inset = 0;
    CGRect frame = CGRectZero;
    CFTimeInterval animationDuration = 0;
};

ClickPulseRenderPlan BuildClickPulseRenderPlan(
    const ClickEffectRenderCommand& command);

void ConfigureClickPulseBaseLayer(
    CAShapeLayer* base,
    NSView* content,
    const ClickPulseRenderPlan& plan);

void AddClickPulseExtraLayers(
    NSView* content,
    const ClickPulseRenderPlan& plan);

void StartClickPulseAnimation(
    CAShapeLayer* base,
    const ClickPulseRenderPlan& plan);

int64_t ComputeClickPulseCloseDelayNs(const ClickPulseRenderPlan& plan);
#endif

} // namespace mousefx::macos_click_pulse
