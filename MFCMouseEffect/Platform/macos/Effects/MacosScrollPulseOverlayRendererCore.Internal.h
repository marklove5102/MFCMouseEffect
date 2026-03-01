#pragma once

#include "MouseFx/Core/Effects/ScrollEffectCompute.h"
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

void AddScrollPulseDecorations(
    NSView* content,
    const ScrollPulseRenderPlan& plan);

void StartScrollPulseAnimation(
    CAShapeLayer* body,
    CAShapeLayer* arrow,
    const ScrollPulseRenderPlan& plan);
#endif

} // namespace mousefx::macos_scroll_pulse
