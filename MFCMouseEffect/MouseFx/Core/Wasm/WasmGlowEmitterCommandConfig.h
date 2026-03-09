#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Core/Wasm/WasmCommandCoordinateSemantics.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace mousefx::wasm {

constexpr uint16_t kMaxGlowEmitterParticles = 512u;
constexpr int kMaxGlowEmitterOverlaySquarePx = 512;

struct ResolvedGlowEmitterCommand final {
    uint32_t emitterId = 0u;
    ScreenPoint screenPt{};
    int frameLeftPx = 0;
    int frameTopPx = 0;
    int squareSizePx = 64;
    float localX = 0.0f;
    float localY = 0.0f;
    float emissionRatePerSec = 96.0f;
    float directionRad = 0.0f;
    float spreadRad = 1.0471976f;
    float speedMin = 140.0f;
    float speedMax = 260.0f;
    float radiusMinPx = 3.0f;
    float radiusMaxPx = 9.0f;
    float alphaMin = 0.18f;
    float alphaMax = 0.90f;
    uint32_t colorArgb = 0xFFFFFFFFu;
    float accelerationX = 0.0f;
    float accelerationY = 0.0f;
    uint32_t emitterTtlMs = 420u;
    uint32_t particleLifeMs = 900u;
    uint16_t maxParticles = 160u;
    bool useGroupLocalOrigin = false;
    RenderSemantics semantics{};
};

inline float ClampGlowEmitterFloat(float value, float fallback, float minValue, float maxValue) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

inline ScreenPoint ClampGlowEmitterScreenPoint(float x, float y) {
    const double clampedX = std::clamp(static_cast<double>(x), -32768.0, 32768.0);
    const double clampedY = std::clamp(static_cast<double>(y), -32768.0, 32768.0);
    return ScreenPoint{
        static_cast<int32_t>(std::lround(clampedX)),
        static_cast<int32_t>(std::lround(clampedY)),
    };
}

inline float NormalizeRadians(float value) {
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    constexpr float kTau = 6.283185307179586f;
    constexpr float kPi = 3.141592653589793f;
    float normalized = std::fmod(value, kTau);
    if (normalized > kPi) {
        normalized -= kTau;
    } else if (normalized < -kPi) {
        normalized += kTau;
    }
    return normalized;
}

inline bool TryResolveUpsertGlowEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    bool overlayMotionYUp,
    ResolvedGlowEmitterCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_glow_emitter command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertGlowEmitterCommandV1)) {
        if (outError) {
            *outError = "upsert_glow_emitter command truncated";
        }
        return false;
    }

    UpsertGlowEmitterCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.emitterId == 0u) {
        if (outError) {
            *outError = "upsert_glow_emitter emitter_id must be non-zero";
        }
        return false;
    }

    ResolvedGlowEmitterCommand resolved{};
    resolved.emitterId = cmd.emitterId;
    resolved.emissionRatePerSec = ClampGlowEmitterFloat(cmd.emissionRatePerSec, 96.0f, 1.0f, 1200.0f);
    resolved.directionRad = NormalizeRadians(overlayMotionYUp ? -cmd.directionRad : cmd.directionRad);
    resolved.spreadRad = ClampGlowEmitterFloat(cmd.spreadRad, 1.0471976f, 0.0f, 6.2831853f);
    resolved.speedMin = ClampGlowEmitterFloat(cmd.speedMin, 140.0f, 1.0f, 2400.0f);
    resolved.speedMax = ClampGlowEmitterFloat(cmd.speedMax, 260.0f, resolved.speedMin, 3200.0f);
    resolved.radiusMinPx = ClampGlowEmitterFloat(cmd.radiusMinPx, 3.0f, 0.5f, 72.0f);
    resolved.radiusMaxPx = ClampGlowEmitterFloat(cmd.radiusMaxPx, 9.0f, resolved.radiusMinPx, 128.0f);
    resolved.alphaMin = ClampGlowEmitterFloat(cmd.alphaMin, 0.18f, 0.01f, 1.0f);
    resolved.alphaMax = ClampGlowEmitterFloat(cmd.alphaMax, 0.90f, resolved.alphaMin, 1.0f);
    resolved.colorArgb = (cmd.colorArgb != 0u) ? cmd.colorArgb : config.icon.fillColor.value;
    resolved.emitterTtlMs = std::clamp<uint32_t>(cmd.emitterTtlMs, 40u, 10000u);
    resolved.particleLifeMs = std::clamp<uint32_t>(cmd.particleLifeMs, 60u, 12000u);
    resolved.maxParticles = std::clamp<uint16_t>(cmd.maxParticles, 1u, kMaxGlowEmitterParticles);
    resolved.useGroupLocalOrigin = (cmd.flags & kUpsertGlowEmitterFlagUseGroupLocalOrigin) != 0u;
    const bool legacyScreenBlend = (cmd.flags & kUpsertGlowEmitterFlagScreenBlend) != 0u;
    if (!TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
            raw,
            sizeBytes,
            sizeof(UpsertGlowEmitterCommandV1),
            legacyScreenBlend,
            &resolved.semantics,
            outError,
            "upsert_glow_emitter")) {
        return false;
    }

    WasmCommandMotion motion{
        0.0f,
        0.0f,
        cmd.accelerationX,
        cmd.accelerationY,
    };
    if (overlayMotionYUp) {
        motion = ConvertMotionToOverlayYUp(motion);
    }
    resolved.accelerationX = ClampGlowEmitterFloat(motion.accelerationX, 0.0f, -4800.0f, 4800.0f);
    resolved.accelerationY = ClampGlowEmitterFloat(motion.accelerationY, 0.0f, -4800.0f, 4800.0f);

    const ScreenPoint screenPoint = ClampGlowEmitterScreenPoint(cmd.x, cmd.y);
    resolved.screenPt = screenPoint;
    const ScreenPoint overlayPoint = ScreenToOverlayPoint(screenPoint);

    const double lifeSec = static_cast<double>(resolved.particleLifeMs) / 1000.0;
    const double accelAbs = std::hypot(
        static_cast<double>(resolved.accelerationX),
        static_cast<double>(resolved.accelerationY));
    const double maxTravel =
        static_cast<double>(resolved.speedMax) * lifeSec +
        0.5 * accelAbs * lifeSec * lifeSec;
    const double particleExtent = maxTravel + static_cast<double>(resolved.radiusMaxPx) * 3.5 + 12.0;
    const int unclampedSidePx = static_cast<int>(std::ceil(std::max(48.0, particleExtent * 2.0)));
    const int sidePx = std::clamp(unclampedSidePx, 64, kMaxGlowEmitterOverlaySquarePx);

    resolved.frameLeftPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.x) - sidePx * 0.5));
    resolved.frameTopPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.y) - sidePx * 0.5));
    resolved.squareSizePx = sidePx;
    resolved.localX = static_cast<float>(overlayPoint.x - resolved.frameLeftPx);
    resolved.localY = static_cast<float>(overlayPoint.y - resolved.frameTopPx);
    *outResolved = resolved;
    return true;
}

inline bool TryResolveRemoveGlowEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    uint32_t* outEmitterId,
    std::string* outError) {
    if (!raw || !outEmitterId) {
        if (outError) {
            *outError = "remove_glow_emitter command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(RemoveGlowEmitterCommandV1)) {
        if (outError) {
            *outError = "remove_glow_emitter command truncated";
        }
        return false;
    }

    RemoveGlowEmitterCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.emitterId == 0u) {
        if (outError) {
            *outError = "remove_glow_emitter emitter_id must be non-zero";
        }
        return false;
    }
    *outEmitterId = cmd.emitterId;
    return true;
}

} // namespace mousefx::wasm
