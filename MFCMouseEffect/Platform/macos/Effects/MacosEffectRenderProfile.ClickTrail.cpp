#include "pch.h"

#include "Platform/macos/Effects/MacosEffectRenderProfile.h"
#include "Platform/macos/Effects/MacosEffectRenderProfile.Shared.h"

#include "MouseFx/Utils/MathUtils.h"

#include <cmath>

namespace mousefx::macos_effect_profile {

ClickRenderProfile ResolveClickRenderProfile(const EffectConfig& config) {
    ClickRenderProfile profile{};
    const int rippleDurationMs = ClampInt(config.ripple.durationMs, 180, 1200);
    const int textDurationMs = ClampInt(config.textClick.durationMs, 220, 1800);
    const int baseWindowSize = ClampInt(config.ripple.windowSize, 80, 220);
    profile.normalSizePx = baseWindowSize + 22;
    profile.textSizePx = baseWindowSize + 38;
    profile.normalDurationSec =
        detail::ClampDouble(static_cast<double>(rippleDurationMs) / 1000.0 * 1.06, 0.20, 1.30);
    profile.textDurationSec =
        detail::ClampDouble(static_cast<double>(textDurationMs) / 1000.0 * 0.50, 0.28, 1.35);
    profile.closePaddingMs = 60;
    profile.baseOpacity = 0.95;
    profile.leftButton.fillArgb = config.ripple.leftClick.fill.value;
    profile.leftButton.strokeArgb = config.ripple.leftClick.stroke.value;
    profile.leftButton.glowArgb = config.ripple.leftClick.glow.value;
    profile.rightButton.fillArgb = config.ripple.rightClick.fill.value;
    profile.rightButton.strokeArgb = config.ripple.rightClick.stroke.value;
    profile.rightButton.glowArgb = config.ripple.rightClick.glow.value;
    profile.middleButton.fillArgb = config.ripple.middleClick.fill.value;
    profile.middleButton.strokeArgb = config.ripple.middleClick.stroke.value;
    profile.middleButton.glowArgb = config.ripple.middleClick.glow.value;
    return profile;
}

TrailRenderProfile ResolveTrailRenderProfile(const EffectConfig& config, const std::string& trailType) {
    TrailRenderProfile profile{};
    const std::string normalizedTrailType = detail::NormalizeTrailTypeAlias(trailType);
    const TrailHistoryProfile history = config.GetTrailHistoryProfile(normalizedTrailType);
    const int durationMs = ClampInt(static_cast<int>(std::lround(history.durationMs * 0.73)), 180, 1000);
    profile.durationSec = static_cast<double>(durationMs) / 1000.0;
    profile.normalSizePx = ClampInt(56 + history.maxPoints / 3, 56, 112);
    profile.particleSizePx = ClampInt(40 + history.maxPoints / 6, 40, 72);
    profile.closePaddingMs = 40;
    profile.baseOpacity = 0.95;
    return profile;
}

TrailThrottleProfile ResolveTrailThrottleProfile(const EffectConfig& config, const std::string& trailType) {
    const std::string normalizedTrailType = detail::NormalizeTrailTypeAlias(trailType);
    const TrailThrottleProfile base = detail::ResolveTrailThrottleProfileByType(normalizedTrailType);
    const TrailHistoryProfile history = config.GetTrailHistoryProfile(normalizedTrailType);

    TrailThrottleProfile result{};
    const double durationScale = detail::ClampDouble(static_cast<double>(history.durationMs) / 300.0, 0.5, 2.0);
    const double pointsBias =
        detail::ClampDouble((32.0 - static_cast<double>(history.maxPoints)) / 64.0, -0.5, 0.8);

    result.minIntervalMs = static_cast<uint64_t>(ClampInt(
        static_cast<int>(std::lround(static_cast<double>(base.minIntervalMs) * std::pow(durationScale, 0.35))),
        8,
        40));
    result.minDistancePx = detail::ClampDouble(base.minDistancePx * (1.0 + pointsBias), 2.0, 12.0);
    return result;
}

ClickRenderProfile DefaultClickRenderProfile() {
    return ResolveClickRenderProfile(EffectConfig::GetDefault());
}

TrailRenderProfile DefaultTrailRenderProfile(const std::string& trailType) {
    return ResolveTrailRenderProfile(EffectConfig::GetDefault(), trailType);
}

TrailThrottleProfile DefaultTrailThrottleProfile(const std::string& trailType) {
    return ResolveTrailThrottleProfile(EffectConfig::GetDefault(), trailType);
}

} // namespace mousefx::macos_effect_profile
