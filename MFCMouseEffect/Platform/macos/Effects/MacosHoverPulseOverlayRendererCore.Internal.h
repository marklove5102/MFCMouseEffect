#pragma once

#include "MouseFx/Core/Effects/HoverEffectCompute.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "Platform/macos/Effects/MacosOverlayRenderSupport.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#ifdef __OBJC__
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#else
struct objc_object;
using NSView = objc_object;
using CAShapeLayer = objc_object;
using NSRect = CGRect;
#endif
#endif

namespace mousefx::macos_hover_pulse {

#if defined(__APPLE__)
struct HoverPulseRenderPlan {
    HoverEffectRenderCommand command{};
    CGFloat size = 0;
    NSRect frame = CGRectZero;
    CFTimeInterval breatheDurationSec = 0;
    CFTimeInterval tubesSpinDurationSec = 0;
};

HoverPulseRenderPlan BuildHoverPulseRenderPlan(const HoverEffectRenderCommand& command);

void ConfigureHoverRingLayer(
    CAShapeLayer* ring,
    NSView* content,
    const HoverPulseRenderPlan& plan);

void AddHoverExtraLayersAndAnimations(
    NSView* content,
    const HoverPulseRenderPlan& plan);
#endif

} // namespace mousefx::macos_hover_pulse
