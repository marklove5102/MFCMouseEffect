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

constexpr uint16_t kMaxParticleEmitterParticles = 512u;
constexpr int kMaxParticleEmitterOverlaySquarePx = 512;

struct ResolvedParticleEmitterCommand final {
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
    uint8_t particleStyle = kParticleEmitterStyleSoftGlow;
    uint8_t emissionMode = kParticleEmitterEmissionModeCone;
    uint8_t spawnShape = kParticleEmitterSpawnShapePoint;
    float spawnRadiusX = 0.0f;
    float spawnRadiusY = 0.0f;
    float spawnInnerRatio = 0.0f;
    float dragPerSecond = 0.0f;
    float turbulenceAccel = 0.0f;
    float turbulenceFrequencyHz = 0.0f;
    float turbulencePhaseJitter = 1.0f;
    bool hasLifeTail = false;
    float sizeStartScale = 1.0f;
    float sizeEndScale = 1.0f;
    float alphaStartScale = 1.0f;
    float alphaEndScale = 1.0f;
    uint32_t colorStartArgb = 0xFFFFFFFFu;
    uint32_t colorEndArgb = 0xFFFFFFFFu;
    bool useGroupLocalOrigin = false;
    RenderSemantics semantics{};
};

inline float ClampParticleEmitterFloat(float value, float fallback, float minValue, float maxValue) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minValue, maxValue);
}

inline ScreenPoint ClampParticleEmitterScreenPoint(float x, float y) {
    const double clampedX = std::clamp(static_cast<double>(x), -32768.0, 32768.0);
    const double clampedY = std::clamp(static_cast<double>(y), -32768.0, 32768.0);
    return ScreenPoint{
        static_cast<int32_t>(std::lround(clampedX)),
        static_cast<int32_t>(std::lround(clampedY)),
    };
}

inline float NormalizeParticleEmitterRadians(float value) {
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

inline uint8_t ResolveParticleEmitterStyle(uint8_t value) {
    switch (value) {
    case kParticleEmitterStyleSoftGlow:
    case kParticleEmitterStyleSolidDisc:
        return value;
    default:
        return kParticleEmitterStyleSoftGlow;
    }
}

inline uint8_t ResolveParticleEmitterEmissionMode(uint8_t value) {
    switch (value) {
    case kParticleEmitterEmissionModeCone:
    case kParticleEmitterEmissionModeRadial:
        return value;
    default:
        return kParticleEmitterEmissionModeCone;
    }
}

inline uint8_t ResolveParticleEmitterSpawnShape(uint8_t value) {
    switch (value) {
    case kParticleEmitterSpawnShapePoint:
    case kParticleEmitterSpawnShapeDisc:
    case kParticleEmitterSpawnShapeRing:
        return value;
    default:
        return kParticleEmitterSpawnShapePoint;
    }
}

inline bool TryResolveUpsertParticleEmitterTails(
    const uint8_t* raw,
    size_t sizeBytes,
    size_t payloadBytes,
    bool legacyScreenBlend,
    ResolvedParticleEmitterCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_particle_emitter tail output is null";
        }
        return false;
    }

    outResolved->hasLifeTail = false;
    outResolved->emissionMode = kParticleEmitterEmissionModeCone;
    outResolved->spawnShape = kParticleEmitterSpawnShapePoint;
    outResolved->spawnRadiusX = 0.0f;
    outResolved->spawnRadiusY = 0.0f;
    outResolved->spawnInnerRatio = 0.0f;
    outResolved->dragPerSecond = 0.0f;
    outResolved->turbulenceAccel = 0.0f;
    outResolved->turbulenceFrequencyHz = 0.0f;
    outResolved->turbulencePhaseJitter = 1.0f;
    outResolved->sizeStartScale = 1.0f;
    outResolved->sizeEndScale = 1.0f;
    outResolved->alphaStartScale = 1.0f;
    outResolved->alphaEndScale = 1.0f;
    outResolved->colorStartArgb = outResolved->colorArgb;
    outResolved->colorEndArgb = outResolved->colorArgb;
    outResolved->semantics.blendMode = legacyScreenBlend ? RenderBlendMode::Screen : RenderBlendMode::Normal;
    outResolved->semantics.sortKey = 0;
    outResolved->semantics.groupId = 0u;
    outResolved->semantics.clipRect = {};

    if (sizeBytes <= payloadBytes) {
        return true;
    }

    const size_t tailBytes = sizeBytes - payloadBytes;
    const size_t kSpawnTailBytes = sizeof(ParticleEmitterSpawnTailV1);
    const size_t kDynamicsTailBytes = sizeof(ParticleEmitterDynamicsTailV1);
    const size_t kLifeTailBytes = sizeof(ParticleEmitterLifeTailV1);
    const size_t kSemanticsTailBytes = sizeof(CommandRenderSemanticsTailV1);
    const size_t kClipTailBytes = sizeof(CommandClipRectTailV1);
    bool hasSpawnTail = false;
    bool hasDynamicsTail = false;
    bool hasLifeTail = false;
    bool hasSemanticsTail = false;
    bool hasClipTail = false;
    bool matchedTailLayout = false;
    for (uint32_t mask = 0u; mask < 32u; ++mask) {
        const bool candidateHasSpawnTail = (mask & 0x01u) != 0u;
        const bool candidateHasDynamicsTail = (mask & 0x02u) != 0u;
        const bool candidateHasLifeTail = (mask & 0x04u) != 0u;
        const bool candidateHasSemanticsTail = (mask & 0x08u) != 0u;
        const bool candidateHasClipTail = (mask & 0x10u) != 0u;
        if (candidateHasClipTail && !candidateHasSemanticsTail) {
            continue;
        }

        size_t candidateBytes = 0u;
        if (candidateHasSpawnTail) {
            candidateBytes += kSpawnTailBytes;
        }
        if (candidateHasDynamicsTail) {
            candidateBytes += kDynamicsTailBytes;
        }
        if (candidateHasLifeTail) {
            candidateBytes += kLifeTailBytes;
        }
        if (candidateHasSemanticsTail) {
            candidateBytes += kSemanticsTailBytes;
        }
        if (candidateHasClipTail) {
            candidateBytes += kClipTailBytes;
        }

        if (candidateBytes != tailBytes) {
            continue;
        }

        hasSpawnTail = candidateHasSpawnTail;
        hasDynamicsTail = candidateHasDynamicsTail;
        hasLifeTail = candidateHasLifeTail;
        hasSemanticsTail = candidateHasSemanticsTail;
        hasClipTail = candidateHasClipTail;
        matchedTailLayout = true;
        break;
    }

    if (!matchedTailLayout) {
        if (outError) {
            *outError = "upsert_particle_emitter tail size is unsupported";
        }
        return false;
    }

    size_t offset = payloadBytes;
    if (hasSpawnTail) {
        ParticleEmitterSpawnTailV1 spawnTail{};
        std::memcpy(&spawnTail, raw + offset, sizeof(spawnTail));
        outResolved->emissionMode = ResolveParticleEmitterEmissionMode(spawnTail.emissionMode);
        outResolved->spawnShape = ResolveParticleEmitterSpawnShape(spawnTail.spawnShape);
        outResolved->spawnRadiusX = ClampParticleEmitterFloat(spawnTail.spawnRadiusX, 0.0f, 0.0f, 192.0f);
        outResolved->spawnRadiusY = ClampParticleEmitterFloat(spawnTail.spawnRadiusY, 0.0f, 0.0f, 192.0f);
        if (outResolved->spawnShape == kParticleEmitterSpawnShapeRing) {
            outResolved->spawnInnerRatio =
                ClampParticleEmitterFloat(spawnTail.spawnInnerRatio, 0.72f, 0.0f, 0.98f);
        } else {
            outResolved->spawnInnerRatio = 0.0f;
        }
        offset += sizeof(spawnTail);
    }

    if (hasDynamicsTail) {
        ParticleEmitterDynamicsTailV1 dynamicsTail{};
        std::memcpy(&dynamicsTail, raw + offset, sizeof(dynamicsTail));
        outResolved->dragPerSecond = ClampParticleEmitterFloat(dynamicsTail.dragPerSecond, 0.0f, 0.0f, 12.0f);
        outResolved->turbulenceAccel = ClampParticleEmitterFloat(dynamicsTail.turbulenceAccel, 0.0f, 0.0f, 4800.0f);
        outResolved->turbulenceFrequencyHz =
            ClampParticleEmitterFloat(dynamicsTail.turbulenceFrequencyHz, 0.0f, 0.0f, 24.0f);
        outResolved->turbulencePhaseJitter =
            ClampParticleEmitterFloat(dynamicsTail.turbulencePhaseJitter, 1.0f, 0.0f, 8.0f);
        offset += sizeof(dynamicsTail);
    }

    if (hasLifeTail) {
        ParticleEmitterLifeTailV1 lifeTail{};
        std::memcpy(&lifeTail, raw + offset, sizeof(lifeTail));
        outResolved->hasLifeTail = true;
        outResolved->sizeStartScale = ClampParticleEmitterFloat(lifeTail.sizeStartScale, 1.0f, 0.05f, 6.0f);
        outResolved->sizeEndScale = ClampParticleEmitterFloat(lifeTail.sizeEndScale, 1.0f, 0.05f, 6.0f);
        outResolved->alphaStartScale = ClampParticleEmitterFloat(lifeTail.alphaStartScale, 1.0f, 0.0f, 2.0f);
        outResolved->alphaEndScale = ClampParticleEmitterFloat(lifeTail.alphaEndScale, 1.0f, 0.0f, 2.0f);
        outResolved->colorStartArgb = lifeTail.colorStartArgb;
        outResolved->colorEndArgb = lifeTail.colorEndArgb;
        offset += sizeof(lifeTail);
    }

    if (hasSemanticsTail) {
        CommandRenderSemanticsTailV1 semanticsTail{};
        std::memcpy(&semanticsTail, raw + offset, sizeof(semanticsTail));
        outResolved->semantics.blendMode = ResolveRenderBlendMode(semanticsTail.blendMode);
        outResolved->semantics.sortKey = semanticsTail.sortKey;
        outResolved->semantics.groupId = semanticsTail.groupId;
        offset += sizeof(semanticsTail);
    }

    if (hasClipTail) {
        CommandClipRectTailV1 clipTail{};
        std::memcpy(&clipTail, raw + offset, sizeof(clipTail));
        outResolved->semantics.clipRect.leftPx = clipTail.leftPx;
        outResolved->semantics.clipRect.topPx = clipTail.topPx;
        outResolved->semantics.clipRect.widthPx = clipTail.widthPx;
        outResolved->semantics.clipRect.heightPx = clipTail.heightPx;
    }

    return true;
}

inline bool TryResolveUpsertParticleEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    const EffectConfig& config,
    bool overlayMotionYUp,
    ResolvedParticleEmitterCommand* outResolved,
    std::string* outError) {
    if (!raw || !outResolved) {
        if (outError) {
            *outError = "upsert_particle_emitter command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(UpsertParticleEmitterCommandV1)) {
        if (outError) {
            *outError = "upsert_particle_emitter command truncated";
        }
        return false;
    }

    UpsertParticleEmitterCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.emitterId == 0u) {
        if (outError) {
            *outError = "upsert_particle_emitter emitter_id must be non-zero";
        }
        return false;
    }

    ResolvedParticleEmitterCommand resolved{};
    resolved.emitterId = cmd.emitterId;
    resolved.emissionRatePerSec = ClampParticleEmitterFloat(cmd.emissionRatePerSec, 96.0f, 1.0f, 1200.0f);
    resolved.directionRad = NormalizeParticleEmitterRadians(overlayMotionYUp ? -cmd.directionRad : cmd.directionRad);
    resolved.spreadRad = ClampParticleEmitterFloat(cmd.spreadRad, 1.0471976f, 0.0f, 6.2831853f);
    resolved.speedMin = ClampParticleEmitterFloat(cmd.speedMin, 140.0f, 1.0f, 2400.0f);
    resolved.speedMax = ClampParticleEmitterFloat(cmd.speedMax, 260.0f, resolved.speedMin, 3200.0f);
    resolved.radiusMinPx = ClampParticleEmitterFloat(cmd.radiusMinPx, 3.0f, 0.5f, 72.0f);
    resolved.radiusMaxPx = ClampParticleEmitterFloat(cmd.radiusMaxPx, 9.0f, resolved.radiusMinPx, 128.0f);
    resolved.alphaMin = ClampParticleEmitterFloat(cmd.alphaMin, 0.18f, 0.01f, 1.0f);
    resolved.alphaMax = ClampParticleEmitterFloat(cmd.alphaMax, 0.90f, resolved.alphaMin, 1.0f);
    resolved.colorArgb = (cmd.colorArgb != 0u) ? cmd.colorArgb : config.icon.fillColor.value;
    resolved.colorStartArgb = resolved.colorArgb;
    resolved.colorEndArgb = resolved.colorArgb;
    resolved.emitterTtlMs = std::clamp<uint32_t>(cmd.emitterTtlMs, 40u, 10000u);
    resolved.particleLifeMs = std::clamp<uint32_t>(cmd.particleLifeMs, 60u, 12000u);
    resolved.maxParticles = std::clamp<uint16_t>(cmd.maxParticles, 1u, kMaxParticleEmitterParticles);
    resolved.particleStyle = ResolveParticleEmitterStyle(cmd.particleStyle);
    resolved.useGroupLocalOrigin = (cmd.flags & kUpsertParticleEmitterFlagUseGroupLocalOrigin) != 0u;
    const bool legacyScreenBlend = (cmd.flags & kUpsertParticleEmitterFlagScreenBlend) != 0u;
    if (!TryResolveUpsertParticleEmitterTails(
            raw,
            sizeBytes,
            sizeof(UpsertParticleEmitterCommandV1),
            legacyScreenBlend,
            &resolved,
            outError)) {
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
    resolved.accelerationX = ClampParticleEmitterFloat(motion.accelerationX, 0.0f, -4800.0f, 4800.0f);
    resolved.accelerationY = ClampParticleEmitterFloat(motion.accelerationY, 0.0f, -4800.0f, 4800.0f);

    const ScreenPoint screenPoint = ClampParticleEmitterScreenPoint(cmd.x, cmd.y);
    resolved.screenPt = screenPoint;
    const ScreenPoint overlayPoint = ScreenToOverlayPoint(screenPoint);

    const double lifeSec = static_cast<double>(resolved.particleLifeMs) / 1000.0;
    const double accelAbs = std::hypot(
        static_cast<double>(resolved.accelerationX),
        static_cast<double>(resolved.accelerationY)) + static_cast<double>(resolved.turbulenceAccel);
    const double maxTravel =
        static_cast<double>(resolved.speedMax) * lifeSec +
        0.5 * accelAbs * lifeSec * lifeSec;
    const double spawnExtent = std::max(
        static_cast<double>(resolved.spawnRadiusX),
        static_cast<double>(resolved.spawnRadiusY));
    const double particleExtent = spawnExtent + maxTravel + static_cast<double>(resolved.radiusMaxPx) * 3.5 + 12.0;
    const int unclampedSidePx = static_cast<int>(std::ceil(std::max(48.0, particleExtent * 2.0)));
    const int sidePx = std::clamp(unclampedSidePx, 64, kMaxParticleEmitterOverlaySquarePx);

    resolved.frameLeftPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.x) - sidePx * 0.5));
    resolved.frameTopPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.y) - sidePx * 0.5));
    resolved.squareSizePx = sidePx;
    resolved.localX = static_cast<float>(overlayPoint.x - resolved.frameLeftPx);
    resolved.localY = static_cast<float>(overlayPoint.y - resolved.frameTopPx);
    *outResolved = resolved;
    return true;
}

inline bool TryResolveRemoveParticleEmitterCommand(
    const uint8_t* raw,
    size_t sizeBytes,
    uint32_t* outEmitterId,
    std::string* outError) {
    if (!raw || !outEmitterId) {
        if (outError) {
            *outError = "remove_particle_emitter command buffer is null";
        }
        return false;
    }
    if (sizeBytes < sizeof(RemoveParticleEmitterCommandV1)) {
        if (outError) {
            *outError = "remove_particle_emitter command truncated";
        }
        return false;
    }

    RemoveParticleEmitterCommandV1 cmd{};
    std::memcpy(&cmd, raw, sizeof(cmd));
    if (cmd.emitterId == 0u) {
        if (outError) {
            *outError = "remove_particle_emitter emitter_id must be non-zero";
        }
        return false;
    }
    *outEmitterId = cmd.emitterId;
    return true;
}

} // namespace mousefx::wasm
