#pragma once

#include "MouseFx/Core/Wasm/WasmGroupMaterialStyle.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mousefx::wasm {

inline uint8_t GroupMaterialTintStrength(uint32_t tintArgb) {
    return static_cast<uint8_t>((tintArgb >> 24) & 0xFFu);
}

inline uint32_t BuildArgb(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) {
    return
        (static_cast<uint32_t>(alpha) << 24) |
        (static_cast<uint32_t>(red) << 16) |
        (static_cast<uint32_t>(green) << 8) |
        static_cast<uint32_t>(blue);
}

inline uint8_t BlendTintChannel(uint8_t baseChannel, uint8_t tintChannel, uint8_t tintStrength) {
    const float strength = static_cast<float>(tintStrength) / 255.0f;
    const float baseValue = static_cast<float>(baseChannel);
    const float tintedValue = baseValue * (static_cast<float>(tintChannel) / 255.0f);
    return static_cast<uint8_t>(std::lround(baseValue + (tintedValue - baseValue) * strength));
}

inline uint8_t ScaleGroupMaterialChannel(uint8_t channel, float intensityMultiplier) {
    const float scaled = static_cast<float>(channel) * ClampGroupMaterialIntensity(intensityMultiplier);
    return static_cast<uint8_t>(std::clamp(std::lround(scaled), 0l, 255l));
}

inline uint32_t ApplyGroupMaterialToArgb(uint32_t baseArgb, const GroupMaterialState& materialState) {
    uint8_t alpha = static_cast<uint8_t>((baseArgb >> 24) & 0xFFu);
    uint8_t red = static_cast<uint8_t>((baseArgb >> 16) & 0xFFu);
    uint8_t green = static_cast<uint8_t>((baseArgb >> 8) & 0xFFu);
    uint8_t blue = static_cast<uint8_t>(baseArgb & 0xFFu);

    if (materialState.hasTintOverride) {
        const uint8_t tintStrength = GroupMaterialTintStrength(materialState.tintArgb);
        const uint8_t tintRed = static_cast<uint8_t>((materialState.tintArgb >> 16) & 0xFFu);
        const uint8_t tintGreen = static_cast<uint8_t>((materialState.tintArgb >> 8) & 0xFFu);
        const uint8_t tintBlue = static_cast<uint8_t>(materialState.tintArgb & 0xFFu);
        red = BlendTintChannel(red, tintRed, tintStrength);
        green = BlendTintChannel(green, tintGreen, tintStrength);
        blue = BlendTintChannel(blue, tintBlue, tintStrength);
    }

    const float intensityMultiplier = ResolveEffectiveGroupMaterialColorIntensity(materialState);
    red = ScaleGroupMaterialChannel(red, intensityMultiplier);
    green = ScaleGroupMaterialChannel(green, intensityMultiplier);
    blue = ScaleGroupMaterialChannel(blue, intensityMultiplier);
    return BuildArgb(alpha, red, green, blue);
}

inline bool GroupMaterialChangesSpriteTint(
    bool applyTint,
    const GroupMaterialState& materialState) {
    return applyTint ||
        materialState.hasTintOverride ||
        std::abs(ResolveEffectiveGroupMaterialColorIntensity(materialState) - 1.0f) > 0.001f;
}

inline uint32_t ApplyGroupMaterialToSpriteTint(
    uint32_t baseTintArgb,
    bool applyTint,
    const GroupMaterialState& materialState) {
    const uint32_t sourceTint = applyTint ? baseTintArgb : 0xFFFFFFFFu;
    const uint32_t tinted = ApplyGroupMaterialToArgb(sourceTint, materialState);
    return 0xFF000000u | (tinted & 0x00FFFFFFu);
}

} // namespace mousefx::wasm
