#pragma once

#include "MouseFx/Core/Wasm/WasmGroupPassRuntime.h"

#include <algorithm>
#include <cmath>

namespace mousefx::wasm {

struct GroupPassStyleProfile final {
    float alphaMultiplier = 1.0f;
    float sizeMultiplier = 1.0f;
    float glowWidthMultiplier = 1.0f;
    float ttlMultiplier = 1.0f;
    float lifeMultiplier = 1.0f;
    float spreadMultiplier = 1.0f;
    float echoDriftPx = 0.0f;
    uint8_t passMode = kGroupPassModeDirectional;
    float phaseRad = 0.0f;
    uint8_t feedbackLayerCount = 1u;
    float feedbackLayerFalloff = 0.5f;
};

struct GroupPassEchoVector final {
    float x = 0.0f;
    float y = 0.0f;
};

inline float LerpGroupPassValue(float startValue, float endValue, float t) {
    return startValue + (endValue - startValue) * t;
}

inline GroupPassStyleProfile BlendGroupPassProfile(
    const GroupPassStyleProfile& baseProfile,
    const GroupPassStyleProfile& stageProfile,
    uint8_t blendMode,
    float blendWeight) {
    const float weight = ClampGroupPassBlendWeight(blendWeight);
    if (weight <= 0.001f) {
        return baseProfile;
    }

    GroupPassStyleProfile combined = baseProfile;
    const uint8_t resolvedBlendMode = ResolveGroupPassBlendMode(blendMode);
    if (resolvedBlendMode == kGroupPassBlendModeLerp) {
        combined.alphaMultiplier = LerpGroupPassValue(baseProfile.alphaMultiplier, stageProfile.alphaMultiplier, weight);
        combined.sizeMultiplier = LerpGroupPassValue(baseProfile.sizeMultiplier, stageProfile.sizeMultiplier, weight);
        combined.glowWidthMultiplier = LerpGroupPassValue(baseProfile.glowWidthMultiplier, stageProfile.glowWidthMultiplier, weight);
        combined.ttlMultiplier = LerpGroupPassValue(baseProfile.ttlMultiplier, stageProfile.ttlMultiplier, weight);
        combined.lifeMultiplier = LerpGroupPassValue(baseProfile.lifeMultiplier, stageProfile.lifeMultiplier, weight);
        combined.spreadMultiplier = LerpGroupPassValue(baseProfile.spreadMultiplier, stageProfile.spreadMultiplier, weight);
        combined.echoDriftPx = LerpGroupPassValue(baseProfile.echoDriftPx, stageProfile.echoDriftPx, weight);
        return combined;
    }

    combined.alphaMultiplier *= LerpGroupPassValue(1.0f, stageProfile.alphaMultiplier, weight);
    combined.sizeMultiplier *= LerpGroupPassValue(1.0f, stageProfile.sizeMultiplier, weight);
    combined.glowWidthMultiplier *= LerpGroupPassValue(1.0f, stageProfile.glowWidthMultiplier, weight);
    combined.ttlMultiplier *= LerpGroupPassValue(1.0f, stageProfile.ttlMultiplier, weight);
    combined.lifeMultiplier *= LerpGroupPassValue(1.0f, stageProfile.lifeMultiplier, weight);
    combined.spreadMultiplier *= LerpGroupPassValue(1.0f, stageProfile.spreadMultiplier, weight);
    combined.echoDriftPx = std::clamp(
        baseProfile.echoDriftPx + stageProfile.echoDriftPx * weight,
        0.0f,
        64.0f);
    return combined;
}

inline GroupPassStyleProfile ResolveSingleGroupPassStyleProfile(
    uint8_t passKind,
    float passAmount,
    float responseAmount) {
    const float amount = ClampGroupPassAmount(passAmount);
    const float response = ClampGroupPassResponseAmount(responseAmount);
    GroupPassStyleProfile profile{};
    switch (ResolveGroupPassKind(passKind)) {
    case kGroupPassKindSoftBloomLike:
        profile.alphaMultiplier = LerpGroupPassValue(1.0f, 1.06f, amount);
        profile.sizeMultiplier = LerpGroupPassValue(1.0f, 1.18f, amount);
        profile.glowWidthMultiplier = LerpGroupPassValue(1.0f, 1.95f, amount);
        profile.spreadMultiplier = LerpGroupPassValue(1.0f, 1.10f, amount * response);
        break;
    case kGroupPassKindAfterimageLike:
        profile.alphaMultiplier = LerpGroupPassValue(1.0f, 0.78f, amount);
        profile.sizeMultiplier = LerpGroupPassValue(1.0f, 1.10f, amount);
        profile.glowWidthMultiplier = LerpGroupPassValue(1.0f, 1.22f, amount);
        profile.ttlMultiplier = LerpGroupPassValue(1.0f, 1.36f, amount);
        profile.lifeMultiplier = LerpGroupPassValue(1.0f, 1.52f, amount);
        profile.echoDriftPx = LerpGroupPassValue(0.0f, 12.0f, amount) * LerpGroupPassValue(0.65f, 1.15f, response);
        break;
    case kGroupPassKindEchoLike:
        profile.alphaMultiplier = LerpGroupPassValue(1.0f, 0.68f, amount);
        profile.sizeMultiplier = LerpGroupPassValue(1.0f, 1.18f, amount);
        profile.glowWidthMultiplier = LerpGroupPassValue(1.0f, 1.36f, amount);
        profile.ttlMultiplier = LerpGroupPassValue(1.0f, 1.50f, amount);
        profile.lifeMultiplier = LerpGroupPassValue(1.0f, 1.66f, amount);
        profile.spreadMultiplier = LerpGroupPassValue(1.0f, 1.30f, amount * LerpGroupPassValue(0.8f, 1.2f, response));
        profile.echoDriftPx = LerpGroupPassValue(0.0f, 28.0f, amount) * LerpGroupPassValue(0.70f, 1.30f, response);
        break;
    default:
        break;
    }
    return profile;
}

inline void ApplyGroupPassStackShaping(
    GroupPassStyleProfile* profile,
    float amount,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff) {
    if (!profile) {
        return;
    }

    const uint8_t resolvedLayerCount = ClampGroupPassFeedbackLayerCount(feedbackLayerCount);
    const float resolvedFalloff = ClampGroupPassFeedbackLayerFalloff(feedbackLayerFalloff);
    const float stackWeight =
        ClampGroupPassAmount(amount) * (static_cast<float>(resolvedLayerCount) - 1.0f) / 3.0f;
    if (stackWeight <= 0.001f) {
        return;
    }

    const float falloffWeight = 1.0f - resolvedFalloff;
    profile->alphaMultiplier *= LerpGroupPassValue(1.0f, 0.74f, stackWeight);
    profile->sizeMultiplier *= LerpGroupPassValue(1.0f, 1.18f, stackWeight * (0.5f + 0.5f * falloffWeight));
    profile->glowWidthMultiplier *= LerpGroupPassValue(1.0f, 1.20f, stackWeight);
    profile->ttlMultiplier *= LerpGroupPassValue(1.0f, 1.32f, stackWeight);
    profile->lifeMultiplier *= LerpGroupPassValue(1.0f, 1.38f, stackWeight);
    profile->spreadMultiplier *= LerpGroupPassValue(1.0f, 1.22f, stackWeight * (0.6f + 0.4f * falloffWeight));
}

inline bool ShouldApplyGroupPassStageToLane(
    const GroupPassStageState& stageState,
    uint8_t laneMaskBit) {
    if ((ResolveGroupPassRouteMask(stageState.routeMask) & laneMaskBit) == 0u) {
        return false;
    }
    return ResolveGroupPassKind(stageState.passKind) != kGroupPassKindNone &&
        ClampGroupPassAmount(stageState.passAmount) > 0.001f;
}

inline float ResolveGroupPassStageLaneResponseMultiplier(
    const GroupPassStageState& stageState,
    uint8_t laneMaskBit) {
    switch (laneMaskBit) {
    case kGroupPassRouteGlow:
        return ClampGroupPassLaneResponseMultiplier(stageState.glowResponseMultiplier);
    case kGroupPassRouteSprite:
        return ClampGroupPassLaneResponseMultiplier(stageState.spriteResponseMultiplier);
    case kGroupPassRouteParticle:
        return ClampGroupPassLaneResponseMultiplier(stageState.particleResponseMultiplier);
    case kGroupPassRouteRibbon:
        return ClampGroupPassLaneResponseMultiplier(stageState.ribbonResponseMultiplier);
    case kGroupPassRouteQuad:
        return ClampGroupPassLaneResponseMultiplier(stageState.quadResponseMultiplier);
    default:
        return 1.0f;
    }
}

inline float ResolveGroupPassTemporalDecayFactor(
    float phaseRateRadPerSec,
    float decayPerSec,
    float decayFloor,
    uint8_t temporalMode,
    float temporalStrength,
    float basePhaseRad,
    float elapsedSec) {
    const float strength = ClampGroupPassTemporalStrength(temporalStrength);
    if (strength <= 0.001f) {
        return 1.0f;
    }
    const float resolvedDecayPerSec = ClampGroupPassDecayPerSec(decayPerSec);
    const uint8_t resolvedTemporalMode = ResolveGroupPassTemporalMode(temporalMode);
    if (resolvedTemporalMode == kGroupPassTemporalModePulse) {
        const float floorValue = ClampGroupPassDecayFloor(decayFloor);
        const float phase = basePhaseRad +
            elapsedSec * ClampGroupPassPhaseRateRadPerSec(phaseRateRadPerSec);
        const float wave = 0.5f + 0.5f * std::sin(phase);
        const float pulsed = floorValue + (1.0f - floorValue) * wave;
        return LerpGroupPassValue(1.0f, pulsed, strength);
    }
    if (resolvedDecayPerSec <= 0.001f) {
        return 1.0f;
    }
    const float floorValue = ClampGroupPassDecayFloor(decayFloor);
    if (resolvedTemporalMode == kGroupPassTemporalModeLinear) {
        const float decay = std::max(0.0f, 1.0f - resolvedDecayPerSec * elapsedSec);
        const float linear = floorValue + (1.0f - floorValue) * decay;
        return LerpGroupPassValue(1.0f, linear, strength);
    }
    const float decay = std::exp(-resolvedDecayPerSec * elapsedSec);
    const float exponential = floorValue + (1.0f - floorValue) * decay;
    return LerpGroupPassValue(1.0f, exponential, strength);
}

inline float ResolveGroupPassStageDecayFactor(
    const GroupPassStageState& stageState,
    float basePhaseRad,
    float elapsedSec) {
    return ResolveGroupPassTemporalDecayFactor(
        stageState.phaseRateRadPerSec,
        stageState.decayPerSec,
        stageState.decayFloor,
        stageState.temporalMode,
        stageState.temporalStrength,
        basePhaseRad,
        elapsedSec);
}

inline GroupPassStyleProfile ResolveGroupPassStyleProfileForLane(
    const GroupPassState& passState,
    uint8_t laneMaskBit) {
    const float amount = ClampGroupPassAmount(passState.passAmount);
    const float elapsedSec = ResolveGroupPassElapsedSec(passState);
    GroupPassStyleProfile profile = ResolveSingleGroupPassStyleProfile(
        passState.passKind,
        passState.passAmount,
        passState.responseAmount);
    if (ShouldApplyGroupPassStageToLane(passState.secondaryStage, laneMaskBit)) {
        const float secondaryDecayFactor = ResolveGroupPassStageDecayFactor(
            passState.secondaryStage,
            passState.phaseRad,
            elapsedSec);
        const float laneResponseAmount = ClampGroupPassResponseAmount(
            passState.secondaryStage.responseAmount *
            ResolveGroupPassStageLaneResponseMultiplier(passState.secondaryStage, laneMaskBit) *
            secondaryDecayFactor);
        const GroupPassStyleProfile secondaryProfile = ResolveSingleGroupPassStyleProfile(
            passState.secondaryStage.passKind,
            ClampGroupPassAmount(passState.secondaryStage.passAmount * secondaryDecayFactor),
            laneResponseAmount);
        profile = BlendGroupPassProfile(
            profile,
            secondaryProfile,
            passState.secondaryStage.blendMode,
            ClampGroupPassBlendWeight(passState.secondaryStage.blendWeight * secondaryDecayFactor));
    }
    if (ShouldApplyGroupPassStageToLane(passState.tertiaryStage, laneMaskBit)) {
        const float tertiaryDecayFactor = ResolveGroupPassStageDecayFactor(
            passState.tertiaryStage,
            passState.phaseRad,
            elapsedSec);
        GroupPassStyleProfile tertiaryProfile = ResolveSingleGroupPassStyleProfile(
            passState.tertiaryStage.passKind,
            ClampGroupPassAmount(passState.tertiaryStage.passAmount * tertiaryDecayFactor),
            ClampGroupPassResponseAmount(
                passState.tertiaryStage.responseAmount *
                ResolveGroupPassStageLaneResponseMultiplier(passState.tertiaryStage, laneMaskBit) *
                tertiaryDecayFactor));
        ApplyGroupPassStackShaping(
            &tertiaryProfile,
            passState.tertiaryStage.passAmount * tertiaryDecayFactor,
            passState.tertiaryStage.feedbackLayerCount,
            passState.tertiaryStage.feedbackLayerFalloff);
        profile = BlendGroupPassProfile(
            profile,
            tertiaryProfile,
            passState.tertiaryStage.blendMode,
            ClampGroupPassBlendWeight(passState.tertiaryStage.blendWeight * tertiaryDecayFactor));
    }
    profile.passMode = ResolveGroupPassMode(passState.passMode);
    profile.phaseRad = NormalizeGroupPassPhaseRad(
        passState.phaseRad +
        elapsedSec * ClampGroupPassPhaseRateRadPerSec(passState.secondaryStage.phaseRateRadPerSec));
    profile.feedbackLayerCount = ClampGroupPassFeedbackLayerCount(passState.feedbackLayerCount);
    profile.feedbackLayerFalloff = ClampGroupPassFeedbackLayerFalloff(passState.feedbackLayerFalloff);
    ApplyGroupPassStackShaping(
        &profile,
        amount,
        profile.feedbackLayerCount,
        profile.feedbackLayerFalloff);
    return profile;
}

inline GroupPassEchoVector ResolveGroupPassEchoDrift(
    float motionX,
    float motionY,
    float fallbackDirectionRad,
    float echoDriftPx,
    uint8_t passMode = kGroupPassModeDirectional,
    float phaseRad = 0.0f,
    uint8_t feedbackLayerCount = 1u,
    float feedbackLayerFalloff = 0.5f) {
    if (echoDriftPx <= 0.001f) {
        return {};
    }

    const float stackSpan = 1.0f +
        (static_cast<float>(ClampGroupPassFeedbackLayerCount(feedbackLayerCount)) - 1.0f) *
            LerpGroupPassValue(0.75f, 0.30f, ClampGroupPassFeedbackLayerFalloff(feedbackLayerFalloff));
    echoDriftPx *= stackSpan;

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

    const uint8_t resolvedMode = ResolveGroupPassMode(passMode);
    if (resolvedMode == kGroupPassModeTangential) {
        const float oldX = dirX;
        dirX = -dirY;
        dirY = oldX;
    } else if (resolvedMode == kGroupPassModeSwirl) {
        const float cosPhase = std::cos(phaseRad);
        const float sinPhase = std::sin(phaseRad);
        const float oldX = dirX;
        const float oldY = dirY;
        dirX = oldX * cosPhase - oldY * sinPhase;
        dirY = oldX * sinPhase + oldY * cosPhase;
    }

    return GroupPassEchoVector{
        -dirX * echoDriftPx,
        -dirY * echoDriftPx,
    };
}

} // namespace mousefx::wasm
