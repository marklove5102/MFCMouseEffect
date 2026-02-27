#include "pch.h"

#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.Shared.h"

#include "MouseFx/Utils/MathUtils.h"

#include <cmath>

namespace mousefx::macos_effect_profile {

ScrollRenderProfile ResolveScrollRenderProfile(const EffectConfig& config) {
    ScrollRenderProfile profile{};
    const int rippleDurationMs = ClampInt(config.ripple.durationMs, 180, 1200);
    profile.baseDurationSec = detail::ClampDouble(static_cast<double>(rippleDurationMs) / 1000.0 * 0.92, 0.24, 1.12);
    profile.perStrengthStepSec = detail::ClampDouble(profile.baseDurationSec * 0.072, 0.012, 0.065);
    profile.horizontalSizePx = ClampInt(config.ripple.windowSize + 34, 116, 236);
    profile.verticalSizePx = ClampInt(config.ripple.windowSize + 24, 108, 224);
    profile.closePaddingMs = 90;
    profile.baseOpacity = 0.96;
    return profile;
}

HoldRenderProfile ResolveHoldRenderProfile(const EffectConfig& config) {
    HoldRenderProfile profile{};
    const int rippleDurationMs = ClampInt(config.ripple.durationMs, 180, 1200);
    profile.sizePx = ClampInt(config.ripple.windowSize + 68, 140, 260);
    profile.progressFullMs = ClampInt(static_cast<int>(std::lround(rippleDurationMs * 4.0)), 800, 3000);
    profile.breatheDurationSec =
        detail::ClampDouble(static_cast<double>(rippleDurationMs) / 1000.0 * 2.57, 0.55, 2.8);
    profile.rotateDurationSec = detail::ClampDouble(profile.breatheDurationSec * 2.44, 1.2, 4.0);
    profile.rotateDurationFastSec = detail::ClampDouble(profile.rotateDurationSec * 0.68, 0.7, 2.4);
    profile.baseOpacity = 0.92;
    return profile;
}

HoverRenderProfile ResolveHoverRenderProfile(const EffectConfig& config) {
    HoverRenderProfile profile{};
    const int rippleDurationMs = ClampInt(config.ripple.durationMs, 180, 1200);
    profile.sizePx = ClampInt(config.ripple.windowSize + 52, 120, 240);
    profile.breatheDurationSec =
        detail::ClampDouble(static_cast<double>(rippleDurationMs) / 1000.0 * 2.43, 0.55, 2.6);
    profile.spinDurationSec = detail::ClampDouble(profile.breatheDurationSec * 1.88, 1.0, 3.8);
    profile.baseOpacity = 0.9;
    return profile;
}

ScrollRenderProfile DefaultScrollRenderProfile() {
    return ResolveScrollRenderProfile(EffectConfig::GetDefault());
}

HoldRenderProfile DefaultHoldRenderProfile() {
    return ResolveHoldRenderProfile(EffectConfig::GetDefault());
}

HoverRenderProfile DefaultHoverRenderProfile() {
    return ResolveHoverRenderProfile(EffectConfig::GetDefault());
}

} // namespace mousefx::macos_effect_profile
