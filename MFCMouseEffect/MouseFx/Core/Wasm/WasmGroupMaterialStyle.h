#pragma once

#include "MouseFx/Core/Wasm/WasmGroupMaterialRuntime.h"

#include <algorithm>
#include <cmath>

namespace mousefx::wasm {

struct GroupMaterialStyleProfile final {
    float colorIntensityMultiplier = 1.0f;
    float alphaMultiplier = 1.0f;
    float sizeMultiplier = 1.0f;
    float glowWidthMultiplier = 1.0f;
    float ttlMultiplier = 1.0f;
    float lifeMultiplier = 1.0f;
    float spreadMultiplier = 1.0f;
    float echoDriftPx = 0.0f;
    uint8_t feedbackMode = kGroupMaterialFeedbackModeDirectional;
    float feedbackPhaseRad = 0.0f;
    uint8_t feedbackLayerCount = 1u;
    float feedbackLayerFalloff = 0.5f;
};

struct GroupMaterialEchoVector final {
    float x = 0.0f;
    float y = 0.0f;
};

inline float LerpGroupMaterialStyleValue(float startValue, float endValue, float t) {
    return startValue + (endValue - startValue) * t;
}

inline GroupMaterialStyleProfile ResolveGroupMaterialStyleProfile(const GroupMaterialState& materialState) {
    const float amount = ClampGroupMaterialStyleAmount(materialState.styleAmount);
    GroupMaterialStyleProfile profile{};
    switch (ResolveGroupMaterialStyleKind(materialState.styleKind)) {
    case kGroupMaterialStyleSoftBloomLike:
        profile = GroupMaterialStyleProfile{
            LerpGroupMaterialStyleValue(1.0f, 1.18f, amount),
            LerpGroupMaterialStyleValue(1.0f, 1.08f, amount),
            LerpGroupMaterialStyleValue(1.0f, 1.22f, amount),
            LerpGroupMaterialStyleValue(1.0f, 1.80f, amount),
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            kGroupMaterialFeedbackModeDirectional,
            0.0f,
            1u,
            0.5f,
        };
        break;
    case kGroupMaterialStyleAfterimageLike:
        profile = GroupMaterialStyleProfile{
            LerpGroupMaterialStyleValue(1.0f, 0.92f, amount),
            LerpGroupMaterialStyleValue(1.0f, 0.56f, amount),
            LerpGroupMaterialStyleValue(1.0f, 1.16f, amount),
            LerpGroupMaterialStyleValue(1.0f, 1.35f, amount),
            LerpGroupMaterialStyleValue(1.0f, 1.28f, amount),
            LerpGroupMaterialStyleValue(1.0f, 1.45f, amount),
            1.0f,
            0.0f,
            kGroupMaterialFeedbackModeDirectional,
            0.0f,
            1u,
            0.5f,
        };
        break;
    default:
        profile = {};
        break;
    }

    const float diffusionAmount = ClampGroupMaterialResponseAmount(materialState.diffusionAmount);
    const float persistenceAmount = ClampGroupMaterialResponseAmount(materialState.persistenceAmount);
    const float echoAmount = ClampGroupMaterialEchoAmount(materialState.echoAmount);
    profile.colorIntensityMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.08f, diffusionAmount);
    profile.alphaMultiplier *= LerpGroupMaterialStyleValue(1.0f, 0.92f, persistenceAmount);
    profile.sizeMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.16f, diffusionAmount);
    profile.glowWidthMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.55f, diffusionAmount);
    profile.ttlMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.45f, persistenceAmount);
    profile.lifeMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.60f, persistenceAmount);
    profile.alphaMultiplier *= LerpGroupMaterialStyleValue(1.0f, 0.82f, echoAmount);
    profile.ttlMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.22f, echoAmount);
    profile.lifeMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.30f, echoAmount);
    profile.spreadMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.28f, echoAmount);
    profile.echoDriftPx = ClampGroupMaterialEchoDriftPx(materialState.echoDriftPx) * echoAmount;
    profile.feedbackMode = ResolveGroupMaterialFeedbackMode(materialState.feedbackMode);
    profile.feedbackPhaseRad = NormalizeGroupMaterialPhaseRad(materialState.feedbackPhaseRad);
    profile.feedbackLayerCount = ClampGroupMaterialFeedbackLayerCount(materialState.feedbackLayerCount);
    profile.feedbackLayerFalloff = ClampGroupMaterialFeedbackLayerFalloff(materialState.feedbackLayerFalloff);
    const float stackWeight =
        echoAmount * (static_cast<float>(profile.feedbackLayerCount) - 1.0f) / 3.0f;
    if (stackWeight > 0.001f) {
        const float falloffWeight = 1.0f - profile.feedbackLayerFalloff;
        profile.alphaMultiplier *= LerpGroupMaterialStyleValue(1.0f, 0.74f, stackWeight);
        profile.sizeMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.18f, stackWeight * (0.5f + 0.5f * falloffWeight));
        profile.glowWidthMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.20f, stackWeight);
        profile.ttlMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.32f, stackWeight);
        profile.lifeMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.38f, stackWeight);
        profile.spreadMultiplier *= LerpGroupMaterialStyleValue(1.0f, 1.22f, stackWeight * (0.6f + 0.4f * falloffWeight));
    }
    return profile;
}

inline float ResolveEffectiveGroupMaterialColorIntensity(const GroupMaterialState& materialState) {
    const GroupMaterialStyleProfile profile = ResolveGroupMaterialStyleProfile(materialState);
    return ClampGroupMaterialIntensity(materialState.intensityMultiplier * profile.colorIntensityMultiplier);
}

inline bool GroupMaterialHasStyleEffect(const GroupMaterialState& materialState) {
    return ResolveGroupMaterialStyleKind(materialState.styleKind) != kGroupMaterialStyleNone &&
        ClampGroupMaterialStyleAmount(materialState.styleAmount) > 0.001f;
}

inline float ApplyGroupMaterialStyleAlpha(float alphaValue, const GroupMaterialState& materialState) {
    const GroupMaterialStyleProfile profile = ResolveGroupMaterialStyleProfile(materialState);
    return std::clamp(alphaValue * profile.alphaMultiplier, 0.0f, 1.0f);
}

inline GroupMaterialEchoVector ResolveGroupMaterialEchoDrift(
    float motionX,
    float motionY,
    float fallbackDirectionRad,
    float echoDriftPx,
    uint8_t feedbackMode = kGroupMaterialFeedbackModeDirectional,
    float phaseRad = 0.0f,
    uint8_t feedbackLayerCount = 1u,
    float feedbackLayerFalloff = 0.5f) {
    float clampedDriftPx = ClampGroupMaterialEchoDriftPx(echoDriftPx);
    if (clampedDriftPx <= 0.001f) {
        return {};
    }

    const float stackSpan = 1.0f +
        (static_cast<float>(ClampGroupMaterialFeedbackLayerCount(feedbackLayerCount)) - 1.0f) *
            LerpGroupMaterialStyleValue(0.75f, 0.30f, ClampGroupMaterialFeedbackLayerFalloff(feedbackLayerFalloff));
    clampedDriftPx *= stackSpan;

    float dirX = motionX;
    float dirY = motionY;
    const float motionLength = std::hypot(dirX, dirY);
    if (motionLength > 0.001f) {
        dirX /= motionLength;
        dirY /= motionLength;
    } else {
        const float fallback = fallbackDirectionRad + phaseRad;
        dirX = std::cos(fallback);
        dirY = std::sin(fallback);
    }

    const uint8_t resolvedMode = ResolveGroupMaterialFeedbackMode(feedbackMode);
    if (resolvedMode == kGroupMaterialFeedbackModeTangential) {
        const float oldX = dirX;
        dirX = -dirY;
        dirY = oldX;
    } else if (resolvedMode == kGroupMaterialFeedbackModeSwirl) {
        const float cosPhase = std::cos(phaseRad);
        const float sinPhase = std::sin(phaseRad);
        const float oldX = dirX;
        const float oldY = dirY;
        dirX = oldX * cosPhase - oldY * sinPhase;
        dirY = oldX * sinPhase + oldY * cosPhase;
    }

    return GroupMaterialEchoVector{
        -dirX * clampedDriftPx,
        -dirY * clampedDriftPx,
    };
}

} // namespace mousefx::wasm
