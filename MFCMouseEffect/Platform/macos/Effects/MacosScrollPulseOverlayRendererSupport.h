#pragma once

#include "Platform/macos/Effects/MacosEffectRenderProfile.h"

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace mousefx::macos_scroll_pulse::support {

int ResolveStrengthLevel(int delta);
CGRect BuildBodyRect(CGFloat size, bool horizontal, int strengthLevel, double intensity);
CFTimeInterval BuildPulseDuration(
    const macos_effect_profile::ScrollRenderProfile& profile,
    int strengthLevel,
    CGFloat overlaySize);
int BuildCloseAfterMs(
    const macos_effect_profile::ScrollRenderProfile& profile,
    CFTimeInterval duration);

} // namespace mousefx::macos_scroll_pulse::support
