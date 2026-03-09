#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Protocol/InputTypes.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"
#include "MouseFx/Styles/RippleStyle.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace mousefx::wasm {

struct ResolvedSpawnPulseCommand final {
    ScreenPoint screenPt{};
    std::string normalizedType{"ripple"};
    float alpha = 1.0f;
    uint32_t delayMs = 0;
    uint32_t lifeMs = 320;
    uint32_t fillColorArgb = 0x594FC3F7u;
    uint32_t strokeColorArgb = 0xFF0288D1u;
    uint32_t glowColorArgb = 0x660288D1u;
    uint32_t fillArgb = 0x594FC3F7u;
    uint32_t strokeArgb = 0xFF0288D1u;
    uint32_t glowArgb = 0x660288D1u;
    RippleStyle style{};
};

inline float ClampSpawnPulseFloat(float value, float fallback, float minValue, float maxValue) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

inline uint32_t ResolveSpawnPulseLifeMs(uint32_t lifeMs, uint32_t fallbackLifeMs) {
    const uint32_t base = (lifeMs > 0u) ? lifeMs : fallbackLifeMs;
    return std::clamp<uint32_t>(base, 80u, 8000u);
}

inline uint32_t ResolveSpawnPulseDelayMs(uint32_t delayMs) {
    return std::min<uint32_t>(delayMs, 60000u);
}

inline float ResolveSpawnPulseAlpha(float alpha) {
    return ClampSpawnPulseFloat(alpha, 1.0f, 0.0f, 1.0f);
}

inline uint32_t ScaleSpawnPulseArgb(uint32_t argb, float alphaScale) {
    const float clampedAlpha = std::clamp(alphaScale, 0.0f, 1.0f);
    const uint32_t baseAlpha = (argb >> 24) & 0xFFu;
    const uint32_t scaledAlpha = static_cast<uint32_t>(
        std::lround(static_cast<double>(baseAlpha) * static_cast<double>(clampedAlpha)));
    return (argb & 0x00FFFFFFu) | ((scaledAlpha & 0xFFu) << 24);
}

inline std::string ResolveSpawnPulseNormalizedType(const SpawnPulseCommandV1& cmd) {
    switch (static_cast<SpawnPulseKind>(cmd.pulseKind)) {
    case SpawnPulseKind::Star:
        return "star";
    case SpawnPulseKind::Ripple:
    default:
        return "ripple";
    }
}

inline ResolvedSpawnPulseCommand ResolveSpawnPulseCommand(
    const EffectConfig& config,
    const SpawnPulseCommandV1& cmd) {
    ResolvedSpawnPulseCommand resolved{};
    resolved.screenPt.x = static_cast<int32_t>(std::lround(cmd.x));
    resolved.screenPt.y = static_cast<int32_t>(std::lround(cmd.y));
    resolved.normalizedType = ResolveSpawnPulseNormalizedType(cmd);
    resolved.alpha = ResolveSpawnPulseAlpha(cmd.alpha);
    resolved.delayMs = ResolveSpawnPulseDelayMs(cmd.delayMs);
    resolved.lifeMs = ResolveSpawnPulseLifeMs(cmd.lifeMs, config.ripple.durationMs);

    RippleStyle style{};
    style.durationMs = resolved.lifeMs;
    style.startRadius = ClampSpawnPulseFloat(cmd.startRadiusPx, config.ripple.startRadius, 0.0f, 640.0f);
    style.endRadius = ClampSpawnPulseFloat(cmd.endRadiusPx, config.ripple.endRadius, 1.0f, 800.0f);
    style.endRadius = std::max(style.endRadius, style.startRadius + 1.0f);
    style.strokeWidth = ClampSpawnPulseFloat(cmd.strokeWidthPx, config.ripple.strokeWidth, 0.1f, 64.0f);

    const bool useThemePalette = (cmd.fillArgb == 0u && cmd.strokeArgb == 0u && cmd.glowArgb == 0u);
    resolved.fillColorArgb = useThemePalette ? config.ripple.leftClick.fill.value : cmd.fillArgb;
    resolved.strokeColorArgb = useThemePalette ? config.ripple.leftClick.stroke.value : cmd.strokeArgb;
    resolved.glowColorArgb = useThemePalette ? config.ripple.leftClick.glow.value : cmd.glowArgb;
    resolved.fillArgb = ScaleSpawnPulseArgb(resolved.fillColorArgb, resolved.alpha);
    resolved.strokeArgb = ScaleSpawnPulseArgb(resolved.strokeColorArgb, resolved.alpha);
    resolved.glowArgb = ScaleSpawnPulseArgb(resolved.glowColorArgb, resolved.alpha);
    style.fill = {resolved.fillArgb};
    style.stroke = {resolved.strokeArgb};
    style.glow = {resolved.glowArgb};

    const float diameter = style.endRadius * 2.0f;
    const float padding = std::max(18.0f, style.strokeWidth * 6.0f);
    style.windowSize = std::clamp<int>(
        static_cast<int>(std::ceil(diameter + padding)),
        32,
        640);
    resolved.style = style;
    return resolved;
}

} // namespace mousefx::wasm
