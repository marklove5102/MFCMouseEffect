#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Core/Wasm/WasmCommandCoordinateSemantics.h"
#include "MouseFx/Core/Wasm/WasmCommandRenderSemanticsTail.h"
#include "MouseFx/Core/Wasm/WasmImageRuntimeConfig.h"
#include "MouseFx/Core/Wasm/WasmPluginAbi.h"
#include "MouseFx/Core/Wasm/WasmPluginImageAssetCatalog.h"
#include "MouseFx/Core/Wasm/WasmRenderValueResolver.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace mousefx::wasm {

constexpr uint16_t kMaxSpriteEmitterParticles = 384u;
constexpr int kMaxSpriteEmitterOverlaySquarePx = 640;

struct ResolvedSpriteEmitterCommand final {
    uint32_t emitterId = 0u;
    uint32_t imageId = 0u;
    std::wstring assetPath{};
    ScreenPoint screenPt{};
    int frameLeftPx = 0;
    int frameTopPx = 0;
    int squareSizePx = 64;
    float localX = 0.0f;
    float localY = 0.0f;
    float emissionRatePerSec = 84.0f;
    float directionRad = 0.0f;
    float spreadRad = 1.0471976f;
    float speedMin = 120.0f;
    float speedMax = 240.0f;
    float sizeMinPx = 24.0f;
    float sizeMaxPx = 72.0f;
    float alphaMin = 0.20f;
    float alphaMax = 0.92f;
    uint32_t tintArgb = 0xFFFFFFFFu;
    bool applyTint = false;
    float rotationMinRad = -0.35f;
    float rotationMaxRad = 0.35f;
    float accelerationX = 0.0f;
    float accelerationY = 0.0f;
    uint32_t emitterTtlMs = 520u;
    uint32_t particleLifeMs = 920u;
    uint16_t maxParticles = 120u;
    bool useGroupLocalOrigin = false;
    RenderSemantics semantics{};
};

inline float ClampSpriteEmitterFloat(float value, float fallback, float minValue, float maxValue) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

inline ScreenPoint ClampSpriteEmitterScreenPoint(float x, float y) {
    const double clampedX = std::clamp(static_cast<double>(x), -32768.0, 32768.0);
    const double clampedY = std::clamp(static_cast<double>(y), -32768.0, 32768.0);
    return ScreenPoint{
        static_cast<int32_t>(std::lround(clampedX)),
        static_cast<int32_t>(std::lround(clampedY)),
    };
}

inline float NormalizeSpriteEmitterRadians(float value, float fallback = 0.0f) {
    if (!std::isfinite(value)) {
        return fallback;
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

inline bool TryResolveUpsertSpriteEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    const std::wstring& activeManifestPath,
    bool overlayMotionYUp,
    ResolvedSpriteEmitterCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_sprite_emitter command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertSpriteEmitterCommandV1)) {
        if (outError) {
            *outError = "upsert_sprite_emitter command truncated";
        }
        return false;
    }

    UpsertSpriteEmitterCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.emitterId == 0u) {
        if (outError) {
            *outError = "upsert_sprite_emitter emitter_id must be non-zero";
        }
        return false;
    }

    ResolvedSpriteEmitterCommand resolved{};
    resolved.emitterId = cmd.emitterId;
    resolved.imageId = cmd.imageId;
    resolved.emissionRatePerSec = ClampSpriteEmitterFloat(cmd.emissionRatePerSec, 84.0f, 1.0f, 1200.0f);
    resolved.directionRad =
        NormalizeSpriteEmitterRadians(overlayMotionYUp ? -cmd.directionRad : cmd.directionRad, 0.0f);
    resolved.spreadRad = ClampSpriteEmitterFloat(cmd.spreadRad, 1.0471976f, 0.0f, 6.2831853f);
    resolved.speedMin = ClampSpriteEmitterFloat(cmd.speedMin, 120.0f, 1.0f, 2400.0f);
    resolved.speedMax = ClampSpriteEmitterFloat(cmd.speedMax, 240.0f, resolved.speedMin, 3200.0f);
    resolved.sizeMinPx = ClampSpriteEmitterFloat(cmd.sizeMinPx, 24.0f, 4.0f, 220.0f);
    resolved.sizeMaxPx = ClampSpriteEmitterFloat(cmd.sizeMaxPx, 72.0f, resolved.sizeMinPx, 320.0f);
    resolved.alphaMin = ClampSpriteEmitterFloat(cmd.alphaMin, 0.20f, 0.01f, 1.0f);
    resolved.alphaMax = ClampSpriteEmitterFloat(cmd.alphaMax, 0.92f, resolved.alphaMin, 1.0f);
    resolved.tintArgb = render_values::ResolveImageTintArgb(config.icon, cmd.tintArgb);
    resolved.applyTint = ResolveSpawnImageApplyTint(cmd.tintArgb);
    resolved.rotationMinRad = NormalizeSpriteEmitterRadians(cmd.rotationMinRad, -0.35f);
    resolved.rotationMaxRad = NormalizeSpriteEmitterRadians(cmd.rotationMaxRad, 0.35f);
    if (resolved.rotationMaxRad < resolved.rotationMinRad) {
        std::swap(resolved.rotationMinRad, resolved.rotationMaxRad);
    }
    resolved.emitterTtlMs = std::clamp<uint32_t>(cmd.emitterTtlMs, 40u, 10000u);
    resolved.particleLifeMs = std::clamp<uint32_t>(cmd.particleLifeMs, 60u, 12000u);
    resolved.maxParticles = std::clamp<uint16_t>(cmd.maxParticles, 1u, kMaxSpriteEmitterParticles);
    resolved.useGroupLocalOrigin = (cmd.flags & kUpsertSpriteEmitterFlagUseGroupLocalOrigin) != 0u;
    const bool legacyScreenBlend = (cmd.flags & kUpsertSpriteEmitterFlagScreenBlend) != 0u;
    if (!TryResolveOptionalCommandRenderSemanticsAndClipRectTail(
            raw,
            sizeBytes,
            sizeof(UpsertSpriteEmitterCommandV1),
            legacyScreenBlend,
            &resolved.semantics,
            outError,
            "upsert_sprite_emitter")) {
        return false;
    }

    WasmPluginImageAssetCatalog::ResolveImageAssetPath(
        activeManifestPath,
        resolved.imageId,
        &resolved.assetPath,
        nullptr);

    WasmCommandMotion motion{
        0.0f,
        0.0f,
        cmd.accelerationX,
        cmd.accelerationY,
    };
    if (overlayMotionYUp) {
        motion = ConvertMotionToOverlayYUp(motion);
    }
    resolved.accelerationX = ClampSpriteEmitterFloat(motion.accelerationX, 0.0f, -4800.0f, 4800.0f);
    resolved.accelerationY = ClampSpriteEmitterFloat(motion.accelerationY, 0.0f, -4800.0f, 4800.0f);

    const ScreenPoint screenPoint = ClampSpriteEmitterScreenPoint(cmd.x, cmd.y);
    resolved.screenPt = screenPoint;
    const ScreenPoint overlayPoint = ScreenToOverlayPoint(screenPoint);

    const double lifeSec = static_cast<double>(resolved.particleLifeMs) / 1000.0;
    const double accelAbs = std::hypot(
        static_cast<double>(resolved.accelerationX),
        static_cast<double>(resolved.accelerationY));
    const double maxTravel =
        static_cast<double>(resolved.speedMax) * lifeSec +
        0.5 * accelAbs * lifeSec * lifeSec;
    const double particleExtent = maxTravel + static_cast<double>(resolved.sizeMaxPx) * 1.4 + 16.0;
    const int unclampedSidePx = static_cast<int>(std::ceil(std::max(72.0, particleExtent * 2.0)));
    const int sidePx = std::clamp(unclampedSidePx, 72, kMaxSpriteEmitterOverlaySquarePx);

    resolved.frameLeftPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.x) - sidePx * 0.5));
    resolved.frameTopPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.y) - sidePx * 0.5));
    resolved.squareSizePx = sidePx;
    resolved.localX = static_cast<float>(overlayPoint.x - resolved.frameLeftPx);
    resolved.localY = static_cast<float>(overlayPoint.y - resolved.frameTopPx);
    *outResolved = resolved;
    return true;
}

inline bool TryResolveRemoveSpriteEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    uint32_t* outEmitterId,
    std::string* outError) {
    if (!raw || !outEmitterId) {
        if (outError) {
            *outError = "remove_sprite_emitter command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(RemoveSpriteEmitterCommandV1)) {
        if (outError) {
            *outError = "remove_sprite_emitter command truncated";
        }
        return false;
    }

    RemoveSpriteEmitterCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.emitterId == 0u) {
        if (outError) {
            *outError = "remove_sprite_emitter emitter_id must be non-zero";
        }
        return false;
    }
    *outEmitterId = cmd.emitterId;
    return true;
}

} // namespace mousefx::wasm
