#include "pch.h"

#include "MouseFx/Core/Wasm/WasmRetainedParticleEmitterRuntime.h"
#include "MouseFx/Core/Wasm/WasmRetainedEmitterRuntimeShared.h"
#include "MouseFx/Core/Wasm/WasmGroupClipRectRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupEffectiveOffset.h"
#include "MouseFx/Core/Wasm/WasmGroupPassRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupPassStyle.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialColor.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupMaterialStyle.h"
#include "MouseFx/Core/Wasm/WasmGroupTransformMath.h"
#include "MouseFx/Core/Wasm/WasmGroupLayerRuntime.h"
#include "MouseFx/Core/Wasm/WasmGroupPresentationRuntime.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "MouseFx/Core/Overlay/OverlayHostService.h"
#include "MouseFx/Interfaces/IRippleRenderer.h"
#include "MouseFx/Renderers/RenderUtils.h"
#include "MouseFx/Styles/RippleStyle.h"
#include "MouseFx/Utils/TimeUtils.h"
#endif

#if MFX_PLATFORM_MACOS
#include "Platform/macos/Wasm/MacosWasmRetainedParticleEmitterSwiftBridge.h"
#endif

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace mousefx::wasm {

namespace {

float LerpFloat(float startValue, float endValue, float t) {
    return startValue + (endValue - startValue) * t;
}

float ApplyVelocityDrag(float velocity, float dragPerSecond, float dtSec) {
    if (dragPerSecond <= 0.0f || dtSec <= 0.0f) {
        return velocity;
    }
    return velocity * std::exp(-dragPerSecond * dtSec);
}

uint32_t LerpArgb(uint32_t startArgb, uint32_t endArgb, float t) {
    const auto lerpChannel = [t](uint8_t startValue, uint8_t endValue) -> uint32_t {
        const float startFloat = static_cast<float>(startValue);
        const float endFloat = static_cast<float>(endValue);
        return static_cast<uint32_t>(std::lround(startFloat + (endFloat - startFloat) * t));
    };

    const uint32_t a = lerpChannel(static_cast<uint8_t>((startArgb >> 24) & 0xFFu), static_cast<uint8_t>((endArgb >> 24) & 0xFFu));
    const uint32_t r = lerpChannel(static_cast<uint8_t>((startArgb >> 16) & 0xFFu), static_cast<uint8_t>((endArgb >> 16) & 0xFFu));
    const uint32_t g = lerpChannel(static_cast<uint8_t>((startArgb >> 8) & 0xFFu), static_cast<uint8_t>((endArgb >> 8) & 0xFFu));
    const uint32_t b = lerpChannel(static_cast<uint8_t>(startArgb & 0xFFu), static_cast<uint8_t>(endArgb & 0xFFu));
    return (a << 24) | (r << 16) | (g << 8) | b;
}

struct RetainedEmitterEntry final {
#if MFX_PLATFORM_MACOS
    void* handle = nullptr;
#elif MFX_PLATFORM_WINDOWS
    uint64_t rippleId = 0u;
    int squareSizePx = 0;
    std::shared_ptr<class WindowsRetainedParticleEmitterSharedState> state{};
#endif
    uint32_t groupId = 0u;
    mousefx::RenderBlendMode blendMode = mousefx::RenderBlendMode::Normal;
    int32_t sortKey = 0;
    mousefx::RenderClipRect clipRect{};
    bool useGroupLocalOrigin = false;
    ResolvedParticleEmitterCommand sourceResolved{};
    int32_t baseFrameLeftPx = 0;
    int32_t baseFrameTopPx = 0;
#if MFX_PLATFORM_WINDOWS
    int32_t baseCenterX = 0;
    int32_t baseCenterY = 0;
#endif
};

using RetainedEmitterStore = RetainedEmitterRuntimeStore<RetainedEmitterEntry>;

ResolvedParticleEmitterCommand ResolveParticleEmitterForGroupTransform(
    const ResolvedParticleEmitterCommand& source,
    const GroupEffectiveTransform& transform) {
    if (!HasGeometryGroupTransform(source.useGroupLocalOrigin, transform.rotationRad, transform.scaleX, transform.scaleY)) {
        return source;
    }

    ResolvedParticleEmitterCommand resolved = source;
    const float scale = AverageGroupTransformScale(transform.scaleX, transform.scaleY);
    const GroupTransformVector transformedCenter = ApplyGroupTransformToPoint(
        static_cast<float>(source.screenPt.x),
        static_cast<float>(source.screenPt.y),
        transform.rotationRad,
        transform.scaleX,
        transform.scaleY,
        transform.pivotXPx,
        transform.pivotYPx);
    const GroupTransformVector transformedAcceleration = ApplyGroupTransformToVector(
        source.accelerationX,
        source.accelerationY,
        transform.rotationRad,
        transform.scaleX,
        transform.scaleY);
    const GroupTransformVector transformedDirection = ApplyGroupTransformToVector(
        std::cos(source.directionRad),
        std::sin(source.directionRad),
        transform.rotationRad,
        transform.scaleX,
        transform.scaleY);
    const float directionScale = std::max(0.05f, std::hypot(transformedDirection.x, transformedDirection.y));

    resolved.directionRad = NormalizeParticleEmitterRadians(std::atan2(transformedDirection.y, transformedDirection.x));
    resolved.speedMin = ClampParticleEmitterFloat(source.speedMin * directionScale, source.speedMin, 1.0f, 2400.0f);
    resolved.speedMax = ClampParticleEmitterFloat(source.speedMax * directionScale, source.speedMax, resolved.speedMin, 3200.0f);
    resolved.radiusMinPx = ClampParticleEmitterFloat(source.radiusMinPx * scale, source.radiusMinPx, 0.5f, 72.0f);
    resolved.radiusMaxPx = ClampParticleEmitterFloat(source.radiusMaxPx * scale, source.radiusMaxPx, resolved.radiusMinPx, 128.0f);
    resolved.accelerationX = ClampParticleEmitterFloat(transformedAcceleration.x, source.accelerationX, -4800.0f, 4800.0f);
    resolved.accelerationY = ClampParticleEmitterFloat(transformedAcceleration.y, source.accelerationY, -4800.0f, 4800.0f);
    resolved.spawnRadiusX = ClampParticleEmitterFloat(
        source.spawnRadiusX * ClampGroupTransformScale(transform.scaleX),
        source.spawnRadiusX,
        0.0f,
        192.0f);
    resolved.spawnRadiusY = ClampParticleEmitterFloat(
        source.spawnRadiusY * ClampGroupTransformScale(transform.scaleY),
        source.spawnRadiusY,
        0.0f,
        192.0f);
    resolved.turbulenceAccel = ClampParticleEmitterFloat(source.turbulenceAccel * scale, source.turbulenceAccel, 0.0f, 4800.0f);

    const ScreenPoint screenPoint = ClampParticleEmitterScreenPoint(transformedCenter.x, transformedCenter.y);
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
    return resolved;
}

ResolvedParticleEmitterCommand ResolveParticleEmitterForGroupMaterial(
    const ResolvedParticleEmitterCommand& source,
    const GroupMaterialState& materialState) {
    ResolvedParticleEmitterCommand resolved = source;
    const GroupMaterialStyleProfile styleProfile = ResolveGroupMaterialStyleProfile(materialState);
    const GroupMaterialEchoVector echoDrift = ResolveGroupMaterialEchoDrift(
        source.accelerationX,
        source.accelerationY,
        source.directionRad,
        styleProfile.echoDriftPx,
        styleProfile.feedbackMode,
        styleProfile.feedbackPhaseRad,
        styleProfile.feedbackLayerCount,
        styleProfile.feedbackLayerFalloff);
    resolved.colorArgb = ApplyGroupMaterialToArgb(source.colorArgb, materialState);
    resolved.radiusMinPx = ClampParticleEmitterFloat(
        source.radiusMinPx * styleProfile.sizeMultiplier,
        source.radiusMinPx,
        0.5f,
        72.0f);
    resolved.radiusMaxPx = ClampParticleEmitterFloat(
        source.radiusMaxPx * styleProfile.sizeMultiplier,
        source.radiusMaxPx,
        resolved.radiusMinPx,
        128.0f);
    resolved.alphaMin = ClampParticleEmitterFloat(
        source.alphaMin * styleProfile.alphaMultiplier,
        source.alphaMin,
        0.01f,
        1.0f);
    resolved.alphaMax = ClampParticleEmitterFloat(
        source.alphaMax * styleProfile.alphaMultiplier,
        source.alphaMax,
        resolved.alphaMin,
        1.0f);
    resolved.emitterTtlMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.emitterTtlMs) * styleProfile.ttlMultiplier)),
        40u,
        10000u);
    resolved.particleLifeMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.particleLifeMs) * styleProfile.lifeMultiplier)),
        60u,
        12000u);
    resolved.spreadRad = ClampParticleEmitterFloat(
        source.spreadRad * styleProfile.spreadMultiplier,
        source.spreadRad,
        0.0f,
        6.2831853f);
    resolved.screenPt = ClampParticleEmitterScreenPoint(
        static_cast<float>(source.screenPt.x) + echoDrift.x,
        static_cast<float>(source.screenPt.y) + echoDrift.y);
    {
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(resolved.screenPt);
        resolved.frameLeftPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.x) - resolved.squareSizePx * 0.5));
        resolved.frameTopPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.y) - resolved.squareSizePx * 0.5));
        resolved.localX = static_cast<float>(overlayPoint.x - resolved.frameLeftPx);
        resolved.localY = static_cast<float>(overlayPoint.y - resolved.frameTopPx);
    }
    if (source.hasLifeTail) {
        resolved.sizeStartScale = ClampParticleEmitterFloat(
            source.sizeStartScale * styleProfile.sizeMultiplier,
            source.sizeStartScale,
            0.05f,
            4.0f);
        resolved.sizeEndScale = ClampParticleEmitterFloat(
            source.sizeEndScale * styleProfile.sizeMultiplier,
            source.sizeEndScale,
            0.05f,
            4.0f);
        resolved.alphaStartScale = ClampParticleEmitterFloat(
            source.alphaStartScale * styleProfile.alphaMultiplier,
            source.alphaStartScale,
            0.0f,
            2.0f);
        resolved.alphaEndScale = ClampParticleEmitterFloat(
            source.alphaEndScale * styleProfile.alphaMultiplier,
            source.alphaEndScale,
            0.0f,
            2.0f);
        resolved.colorStartArgb = ApplyGroupMaterialToArgb(source.colorStartArgb, materialState);
        resolved.colorEndArgb = ApplyGroupMaterialToArgb(source.colorEndArgb, materialState);
    }
    return resolved;
}

ResolvedParticleEmitterCommand ResolveParticleEmitterForGroupPass(
    const ResolvedParticleEmitterCommand& source,
    const GroupPassState& passState) {
    ResolvedParticleEmitterCommand resolved = source;
    const GroupPassStyleProfile passProfile = ResolveGroupPassStyleProfileForLane(
        passState,
        kGroupPassRouteParticle);
    const GroupPassEchoVector echoDrift = ResolveGroupPassEchoDrift(
        source.accelerationX,
        source.accelerationY,
        source.directionRad,
        passProfile.echoDriftPx,
        passProfile.passMode,
        passProfile.phaseRad,
        passProfile.feedbackLayerCount,
        passProfile.feedbackLayerFalloff);
    resolved.radiusMinPx = ClampParticleEmitterFloat(
        source.radiusMinPx * passProfile.sizeMultiplier,
        source.radiusMinPx,
        0.5f,
        72.0f);
    resolved.radiusMaxPx = ClampParticleEmitterFloat(
        source.radiusMaxPx * passProfile.sizeMultiplier,
        source.radiusMaxPx,
        resolved.radiusMinPx,
        128.0f);
    resolved.alphaMin = ClampParticleEmitterFloat(
        source.alphaMin * passProfile.alphaMultiplier,
        source.alphaMin,
        0.01f,
        1.0f);
    resolved.alphaMax = ClampParticleEmitterFloat(
        source.alphaMax * passProfile.alphaMultiplier,
        source.alphaMax,
        resolved.alphaMin,
        1.0f);
    resolved.emitterTtlMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.emitterTtlMs) * passProfile.ttlMultiplier)),
        40u,
        10000u);
    resolved.particleLifeMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.particleLifeMs) * passProfile.lifeMultiplier)),
        60u,
        12000u);
    resolved.spreadRad = ClampParticleEmitterFloat(
        source.spreadRad * passProfile.spreadMultiplier,
        source.spreadRad,
        0.0f,
        6.2831853f);
    resolved.screenPt = ClampParticleEmitterScreenPoint(
        static_cast<float>(source.screenPt.x) + echoDrift.x,
        static_cast<float>(source.screenPt.y) + echoDrift.y);
    {
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(resolved.screenPt);
        resolved.frameLeftPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.x) - resolved.squareSizePx * 0.5));
        resolved.frameTopPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.y) - resolved.squareSizePx * 0.5));
        resolved.localX = static_cast<float>(overlayPoint.x - resolved.frameLeftPx);
        resolved.localY = static_cast<float>(overlayPoint.y - resolved.frameTopPx);
    }
    if (source.hasLifeTail) {
        resolved.sizeStartScale = ClampParticleEmitterFloat(
            source.sizeStartScale * passProfile.sizeMultiplier,
            source.sizeStartScale,
            0.05f,
            8.0f);
        resolved.sizeEndScale = ClampParticleEmitterFloat(
            source.sizeEndScale * passProfile.sizeMultiplier,
            source.sizeEndScale,
            0.05f,
            8.0f);
    }
    return resolved;
}

ResolvedParticleEmitterCommand ResolveEntryParticleEmitterCommand(
    const std::wstring& activeManifestPath,
    const RetainedEmitterEntry& entry) {
    return ResolveParticleEmitterForGroupTransform(
        ResolveParticleEmitterForGroupPass(
            ResolveParticleEmitterForGroupMaterial(
                entry.sourceResolved,
                ResolveGroupMaterial(activeManifestPath, entry.groupId)),
            ResolveGroupPass(activeManifestPath, entry.groupId)),
        ResolveEffectiveGroupTransform(activeManifestPath, entry.groupId, entry.useGroupLocalOrigin));
}

struct TurbulenceVector final {
    float x = 0.0f;
    float y = 0.0f;
};

TurbulenceVector ComputeParticleTurbulence(
    float ageSec,
    float phaseSeed,
    float turbulenceAccel,
    float turbulenceFrequencyHz,
    float turbulencePhaseJitter) {
    if (turbulenceAccel <= 0.0f || turbulenceFrequencyHz <= 0.0f) {
        return {};
    }
    constexpr float kTau = 6.283185307179586f;
    const float jitter = std::max(0.0f, turbulencePhaseJitter);
    const float phase = ageSec * turbulenceFrequencyHz * kTau + phaseSeed * std::max(1.0f, jitter) * kTau;
    const float secondaryPhase = phase * 1.6180339f + phaseSeed * 0.5f * kTau;
    return TurbulenceVector{
        turbulenceAccel * std::sin(phase),
        turbulenceAccel * std::cos(secondaryPhase),
    };
}

#if MFX_PLATFORM_MACOS
bool IsHandleActive(void* handle) {
    return handle != nullptr && mfx_macos_wasm_retained_particle_emitter_is_active_v1(handle) != 0;
}

void ReleaseHandle(void* handle) {
    if (handle != nullptr) {
        mfx_macos_wasm_retained_particle_emitter_release_v1(handle);
    }
}

void UpsertHandle(void* handle, const ResolvedParticleEmitterCommand& resolved) {
    mfx_macos_wasm_retained_particle_emitter_upsert_v1(
        handle,
        resolved.frameLeftPx,
        resolved.frameTopPx,
        resolved.squareSizePx,
        resolved.localX,
        resolved.localY,
        resolved.emissionRatePerSec,
        resolved.directionRad,
        resolved.spreadRad,
        resolved.speedMin,
        resolved.speedMax,
        resolved.radiusMinPx,
        resolved.radiusMaxPx,
        resolved.alphaMin,
        resolved.alphaMax,
        resolved.colorArgb,
        resolved.accelerationX,
        resolved.accelerationY,
        resolved.emitterTtlMs,
        resolved.particleLifeMs,
        resolved.maxParticles,
        resolved.particleStyle,
        resolved.emissionMode,
        resolved.spawnShape,
        resolved.spawnRadiusX,
        resolved.spawnRadiusY,
        resolved.spawnInnerRatio,
        resolved.dragPerSecond,
        resolved.turbulenceAccel,
        resolved.turbulenceFrequencyHz,
        resolved.turbulencePhaseJitter,
        resolved.sizeStartScale,
        resolved.sizeEndScale,
        resolved.alphaStartScale,
        resolved.alphaEndScale,
        resolved.colorStartArgb,
        resolved.colorEndArgb,
        static_cast<uint32_t>(resolved.semantics.blendMode),
        resolved.semantics.sortKey,
        resolved.semantics.groupId,
        resolved.semantics.clipRect.leftPx,
        resolved.semantics.clipRect.topPx,
        resolved.semantics.clipRect.widthPx,
        resolved.semantics.clipRect.heightPx);
}

void ApplyHandleGroupPresentation(void* handle, const GroupPresentationState& presentation) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_particle_emitter_set_group_presentation_v1(
        handle,
        presentation.alphaMultiplier,
        presentation.visible ? 1u : 0u);
}

void ApplyHandleGroupClipRect(
    void* handle,
    const mousefx::RenderClipRect& clipRect,
    uint8_t maskShapeKind,
    float cornerRadiusPx) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_particle_emitter_set_effective_clip_rect_v2(
        handle,
        clipRect.leftPx,
        clipRect.topPx,
        clipRect.widthPx,
        clipRect.heightPx,
        static_cast<uint32_t>(maskShapeKind),
        cornerRadiusPx);
}

void ApplyHandleGroupLayer(void* handle, mousefx::RenderBlendMode blendMode, int32_t sortKey) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_particle_emitter_set_effective_layer_v1(
        handle,
        static_cast<uint32_t>(blendMode),
        sortKey);
}

void ApplyHandleGroupTransform(void* handle, float offsetXPx, float offsetYPx) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_particle_emitter_set_effective_translation_v1(handle, offsetXPx, offsetYPx);
}
#elif MFX_PLATFORM_WINDOWS
class WindowsRetainedParticleEmitterSharedState final {
public:
    void Upsert(const ResolvedParticleEmitterCommand& resolved) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = resolved;
        stopRequested_ = false;
        alive_ = true;
        expireTickMs_ = NowMs() + static_cast<uint64_t>(resolved.emitterTtlMs);
        if (rngState_ == 0u) {
            rngState_ = resolved.emitterId ^ 0x7F4A7C15u;
            if (rngState_ == 0u) {
                rngState_ = 0xA341316Cu;
            }
        }
    }

    void RequestStop() {
        std::lock_guard<std::mutex> lock(mutex_);
        stopRequested_ = true;
        expireTickMs_ = 0u;
    }

    bool IsAlive() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return alive_;
    }

    void Render(Gdiplus::Graphics& g) {
        struct DrawParticle final {
            float x = 0.0f;
            float y = 0.0f;
            float radius = 0.0f;
            float alpha = 0.0f;
            uint32_t colorArgb = 0xFFFFFFFFu;
        };

        std::vector<DrawParticle> drawParticles;
        RenderBlendMode blendMode = RenderBlendMode::Normal;
        uint8_t particleStyle = kParticleEmitterStyleSoftGlow;
        bool hasLifeTail = false;
        float dragPerSecond = 0.0f;
        float turbulenceAccel = 0.0f;
        float turbulenceFrequencyHz = 0.0f;
        float turbulencePhaseJitter = 1.0f;
        float sizeStartScale = 1.0f;
        float sizeEndScale = 1.0f;
        float alphaStartScale = 1.0f;
        float alphaEndScale = 1.0f;
        uint32_t colorStartArgb = 0xFFFFFFFFu;
        uint32_t colorEndArgb = 0xFFFFFFFFu;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!alive_) {
                return;
            }

            const uint64_t nowMs = NowMs();
            if (lastTickMs_ == 0u) {
                lastTickMs_ = nowMs;
            }
            const uint64_t deltaMs = std::clamp<uint64_t>(
                (nowMs >= lastTickMs_) ? (nowMs - lastTickMs_) : 0u,
                1u,
                100u);
            lastTickMs_ = nowMs;
            const float dtSec = static_cast<float>(deltaMs) / 1000.0f;
            blendMode = config_.semantics.blendMode;
            particleStyle = config_.particleStyle;
            dragPerSecond = config_.dragPerSecond;
            turbulenceAccel = config_.turbulenceAccel;
            turbulenceFrequencyHz = config_.turbulenceFrequencyHz;
            turbulencePhaseJitter = config_.turbulencePhaseJitter;
            hasLifeTail = config_.hasLifeTail;
            sizeStartScale = config_.sizeStartScale;
            sizeEndScale = config_.sizeEndScale;
            alphaStartScale = config_.alphaStartScale;
            alphaEndScale = config_.alphaEndScale;
            colorStartArgb = config_.colorStartArgb;
            colorEndArgb = config_.colorEndArgb;

            if (!stopRequested_ && nowMs < expireTickMs_) {
                emitAccumulator_ += config_.emissionRatePerSec * dtSec;
                uint32_t emitCount = static_cast<uint32_t>(std::floor(emitAccumulator_));
                if (emitCount > 0u) {
                    emitAccumulator_ -= static_cast<float>(emitCount);
                    const size_t available =
                        (particles_.size() < config_.maxParticles)
                            ? static_cast<size_t>(config_.maxParticles - static_cast<uint16_t>(particles_.size()))
                            : 0u;
                    emitCount = static_cast<uint32_t>(std::min<size_t>(emitCount, available));
                    particles_.reserve(particles_.size() + emitCount);
                    for (uint32_t index = 0; index < emitCount; ++index) {
                        particles_.push_back(SpawnParticleLocked());
                    }
                }
            }

            std::vector<Particle> updated;
            updated.reserve(particles_.size());
            for (Particle particle : particles_) {
                const TurbulenceVector turbulence = ComputeParticleTurbulence(
                    particle.ageSec,
                    particle.phaseSeed,
                    turbulenceAccel,
                    turbulenceFrequencyHz,
                    turbulencePhaseJitter);
                const float totalAx = config_.accelerationX + turbulence.x;
                const float totalAy = config_.accelerationY + turbulence.y;
                particle.x += particle.vx * dtSec + 0.5f * totalAx * dtSec * dtSec;
                particle.y += particle.vy * dtSec + 0.5f * totalAy * dtSec * dtSec;
                particle.vx += totalAx * dtSec;
                particle.vy += totalAy * dtSec;
                particle.vx = ApplyVelocityDrag(particle.vx, dragPerSecond, dtSec);
                particle.vy = ApplyVelocityDrag(particle.vy, dragPerSecond, dtSec);
                particle.ageSec += dtSec;
                if (particle.ageSec >= particle.lifeSec) {
                    continue;
                }
                updated.push_back(particle);
            }
            particles_.swap(updated);

            if ((stopRequested_ || nowMs >= expireTickMs_) && particles_.empty()) {
                alive_ = false;
                return;
            }

            drawParticles.reserve(particles_.size());
            for (const Particle& particle : particles_) {
                const float progress = std::clamp(particle.ageSec / std::max(0.01f, particle.lifeSec), 0.0f, 1.0f);
                const float life = 1.0f - progress;
                float radius = particle.baseRadius;
                float alpha = particle.baseAlpha * std::pow(life, 0.62f);
                uint32_t colorArgb = particle.colorArgb;
                if (hasLifeTail) {
                    radius = particle.baseRadius * LerpFloat(sizeStartScale, sizeEndScale, progress);
                    alpha = particle.baseAlpha * LerpFloat(alphaStartScale, alphaEndScale, progress);
                    colorArgb = LerpArgb(colorStartArgb, colorEndArgb, progress);
                }
                if (life <= 0.001f || alpha <= 0.001f || radius <= 0.0f) {
                    continue;
                }
                drawParticles.push_back(DrawParticle{
                    particle.x,
                    particle.y,
                    radius,
                    alpha,
                    colorArgb,
                });
            }
        }

        if (drawParticles.empty()) {
            return;
        }

        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        const bool screenBlend = UsesScreenLikeBlend(blendMode);
        for (const DrawParticle& particle : drawParticles) {
            const Gdiplus::Color base = render_utils::ToGdiPlus({particle.colorArgb});
            const BYTE coreAlpha = render_utils::ClampByte(static_cast<int>(base.GetA() * particle.alpha));
            if (coreAlpha == 0u) {
                continue;
            }

            if (particleStyle == kParticleEmitterStyleSolidDisc) {
                const float glowRadius = std::max(1.0f, particle.radius * (screenBlend ? 1.95f : 1.55f));
                Gdiplus::SolidBrush glowBrush(Gdiplus::Color(
                    render_utils::ClampByte(static_cast<int>(base.GetA() * particle.alpha * (screenBlend ? 0.22f : 0.14f))),
                    base.GetR(),
                    base.GetG(),
                    base.GetB()));
                g.FillEllipse(
                    &glowBrush,
                    particle.x - glowRadius,
                    particle.y - glowRadius,
                    glowRadius * 2.0f,
                    glowRadius * 2.0f);

                Gdiplus::SolidBrush coreBrush(Gdiplus::Color(
                    coreAlpha,
                    base.GetR(),
                    base.GetG(),
                    base.GetB()));
                g.FillEllipse(
                    &coreBrush,
                    particle.x - particle.radius,
                    particle.y - particle.radius,
                    particle.radius * 2.0f,
                    particle.radius * 2.0f);
                continue;
            }

            const float glowRadius = std::max(1.0f, particle.radius * (screenBlend ? 3.2f : 2.6f));
            Gdiplus::SolidBrush glowBrush(Gdiplus::Color(
                render_utils::ClampByte(static_cast<int>(base.GetA() * particle.alpha * (screenBlend ? 0.24f : 0.16f))),
                base.GetR(),
                base.GetG(),
                base.GetB()));
            g.FillEllipse(
                &glowBrush,
                particle.x - glowRadius,
                particle.y - glowRadius,
                glowRadius * 2.0f,
                glowRadius * 2.0f);

            const float midRadius = std::max(0.8f, particle.radius * 1.35f);
            Gdiplus::SolidBrush midBrush(Gdiplus::Color(
                render_utils::ClampByte(static_cast<int>(base.GetA() * particle.alpha * 0.34f)),
                base.GetR(),
                base.GetG(),
                base.GetB()));
            g.FillEllipse(
                &midBrush,
                particle.x - midRadius,
                particle.y - midRadius,
                midRadius * 2.0f,
                midRadius * 2.0f);

            Gdiplus::SolidBrush coreBrush(Gdiplus::Color(
                coreAlpha,
                base.GetR(),
                base.GetG(),
                base.GetB()));
            g.FillEllipse(
                &coreBrush,
                particle.x - particle.radius,
                particle.y - particle.radius,
                particle.radius * 2.0f,
                particle.radius * 2.0f);

            const float hotRadius = std::max(0.6f, particle.radius * 0.36f);
            Gdiplus::SolidBrush hotBrush(Gdiplus::Color(
                render_utils::ClampByte(static_cast<int>(coreAlpha * 0.78f)),
                255u,
                255u,
                255u));
            g.FillEllipse(
                &hotBrush,
                particle.x - hotRadius,
                particle.y - hotRadius,
                hotRadius * 2.0f,
                hotRadius * 2.0f);
        }
    }

private:
    struct Particle final {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float baseRadius = 1.0f;
        float baseAlpha = 1.0f;
        uint32_t colorArgb = 0xFFFFFFFFu;
        float phaseSeed = 0.0f;
        float ageSec = 0.0f;
        float lifeSec = 1.0f;
    };

    float NextRandom01Locked() {
        rngState_ ^= (rngState_ << 13);
        rngState_ ^= (rngState_ >> 17);
        rngState_ ^= (rngState_ << 5);
        return static_cast<float>(static_cast<double>(rngState_) / static_cast<double>(UINT32_MAX));
    }

    float RandomLocked(float minValue, float maxValue) {
        return minValue + (maxValue - minValue) * NextRandom01Locked();
    }

    Particle SpawnParticleLocked() {
        float spawnOffsetX = 0.0f;
        float spawnOffsetY = 0.0f;
        if (config_.spawnShape == kParticleEmitterSpawnShapeDisc ||
            config_.spawnShape == kParticleEmitterSpawnShapeRing) {
            constexpr float kTau = 6.283185307179586f;
            const float spawnAngle = RandomLocked(0.0f, kTau);
            const float innerRatio =
                (config_.spawnShape == kParticleEmitterSpawnShapeRing)
                    ? std::clamp(config_.spawnInnerRatio, 0.0f, 0.98f)
                    : 0.0f;
            const float minRadiusSq = innerRatio * innerRatio;
            const float radialScale = std::sqrt(minRadiusSq + (1.0f - minRadiusSq) * NextRandom01Locked());
            spawnOffsetX = std::cos(spawnAngle) * config_.spawnRadiusX * radialScale;
            spawnOffsetY = std::sin(spawnAngle) * config_.spawnRadiusY * radialScale;
        }

        const float halfSpread = config_.spreadRad * 0.5f;
        float angle = config_.directionRad + RandomLocked(-halfSpread, halfSpread);
        if (config_.emissionMode == kParticleEmitterEmissionModeRadial &&
            (std::fabs(spawnOffsetX) > 0.001f || std::fabs(spawnOffsetY) > 0.001f)) {
            angle = std::atan2(spawnOffsetY, spawnOffsetX) + RandomLocked(-halfSpread, halfSpread);
        }
        const float speed = RandomLocked(config_.speedMin, config_.speedMax);
        Particle particle{};
        particle.x = config_.localX + spawnOffsetX;
        particle.y = config_.localY + spawnOffsetY;
        particle.vx = std::cos(angle) * speed;
        particle.vy = std::sin(angle) * speed;
        particle.baseRadius = RandomLocked(config_.radiusMinPx, config_.radiusMaxPx);
        particle.baseAlpha = RandomLocked(config_.alphaMin, config_.alphaMax);
        particle.colorArgb = config_.colorArgb;
        particle.phaseSeed = NextRandom01Locked();
        particle.lifeSec = std::max(0.06f, static_cast<float>(config_.particleLifeMs) / 1000.0f);
        return particle;
    }

    mutable std::mutex mutex_{};
    ResolvedParticleEmitterCommand config_{};
    std::vector<Particle> particles_{};
    float emitAccumulator_ = 0.0f;
    uint64_t lastTickMs_ = 0u;
    uint64_t expireTickMs_ = 0u;
    uint32_t rngState_ = 0x7F4A7C15u;
    bool stopRequested_ = false;
    bool alive_ = true;
};

class WindowsRetainedParticleEmitterRenderer final : public IRippleRenderer {
public:
    explicit WindowsRetainedParticleEmitterRenderer(std::shared_ptr<WindowsRetainedParticleEmitterSharedState> state)
        : state_(std::move(state)) {}

    void Render(
        Gdiplus::Graphics& g,
        float,
        uint64_t,
        int,
        const RippleStyle&) override {
        if (state_) {
            state_->Render(g);
        }
    }

    bool IsAlive() const override {
        return state_ && state_->IsAlive();
    }

private:
    std::shared_ptr<WindowsRetainedParticleEmitterSharedState> state_{};
};

bool IsHandleActive(const RetainedEmitterEntry& entry) {
    if (entry.rippleId == 0u || !entry.state) {
        return false;
    }
    if (!entry.state->IsAlive()) {
        OverlayHostService::Instance().StopRipple(entry.rippleId);
        return false;
    }
    return OverlayHostService::Instance().IsRippleActive(entry.rippleId);
}

void ReleaseHandle(RetainedEmitterEntry* entry) {
    if (!entry) {
        return;
    }
    if (entry->state) {
        entry->state->RequestStop();
    }
    if (entry->rippleId != 0u) {
        OverlayHostService::Instance().StopRipple(entry->rippleId);
    }
    entry->rippleId = 0u;
    entry->squareSizePx = 0;
    entry->state.reset();
}

bool CreateHandle(
    RetainedEmitterEntry* entry,
    const ResolvedParticleEmitterCommand& resolved,
    std::string* outError) {
    if (!entry) {
        if (outError) {
            *outError = "retained particle emitter entry is null";
        }
        return false;
    }

    auto state = std::make_shared<WindowsRetainedParticleEmitterSharedState>();
    state->Upsert(resolved);

    ClickEvent ev{};
    ev.button = MouseButton::Left;
    ev.pt = resolved.screenPt;

    RippleStyle style{};
    style.durationMs = 1u;
    style.windowSize = resolved.squareSizePx;
    style.startRadius = 0.0f;
    style.endRadius = 0.0f;
    style.strokeWidth = 0.0f;
    style.fill = {0u};
    style.stroke = {0u};
    style.glow = {0u};

    RenderParams params{};
    params.loop = false;
    params.intensity = 1.0f;
    params.semantics = resolved.semantics;

    const uint64_t rippleId = OverlayHostService::Instance().ShowContinuousRipple(
        ev,
        style,
        std::make_unique<WindowsRetainedParticleEmitterRenderer>(state),
        params);
    if (rippleId == 0u) {
        if (outError) {
            *outError = "failed to create retained particle emitter overlay";
        }
        return false;
    }

    entry->rippleId = rippleId;
    entry->squareSizePx = resolved.squareSizePx;
    entry->state = std::move(state);
    entry->groupId = resolved.semantics.groupId;
    return true;
}

void UpsertHandle(RetainedEmitterEntry* entry, const ResolvedParticleEmitterCommand& resolved) {
    if (!entry || entry->rippleId == 0u || !entry->state) {
        return;
    }
    entry->state->Upsert(resolved);
    OverlayHostService::Instance().UpdateRipplePosition(entry->rippleId, resolved.screenPt);
}

void ApplyHandleGroupPresentation(RetainedEmitterEntry* entry, const GroupPresentationState&) {
    (void)entry;
}

void ApplyHandleGroupClipRect(RetainedEmitterEntry* entry, const mousefx::RenderClipRect&, uint8_t, float) {
    (void)entry;
}

void ApplyHandleGroupLayer(RetainedEmitterEntry* entry, mousefx::RenderBlendMode, int32_t) {
    (void)entry;
}

void ApplyHandleGroupTransform(RetainedEmitterEntry* entry, float offsetXPx, float offsetYPx) {
    if (!entry || entry->rippleId == 0u) {
        return;
    }
    const ScreenPoint translated{
        static_cast<LONG>(std::lround(static_cast<double>(entry->baseCenterX) + static_cast<double>(offsetXPx))),
        static_cast<LONG>(std::lround(static_cast<double>(entry->baseCenterY) + static_cast<double>(offsetYPx))),
    };
    OverlayHostService::Instance().UpdateRipplePosition(entry->rippleId, translated);
}
#endif

GroupEffectiveOffset ResolveEntryGroupOffset(
    const std::wstring& activeManifestPath,
    const RetainedEmitterEntry& entry) {
    return ResolveEffectiveGroupOffset(activeManifestPath, entry.groupId, entry.useGroupLocalOrigin);
}

void PruneInactiveRetainedEmittersLocked() {
    ::mousefx::wasm::PruneInactiveRetainedEmittersLocked<RetainedEmitterStore>(
        [](const RetainedEmitterEntry& entry) {
#if MFX_PLATFORM_MACOS
            return IsHandleActive(entry.handle);
#elif MFX_PLATFORM_WINDOWS
            return IsHandleActive(entry);
#else
            return true;
#endif
        },
        [](RetainedEmitterEntry* entry) {
#if MFX_PLATFORM_MACOS
            ReleaseHandle(entry ? entry->handle : nullptr);
#elif MFX_PLATFORM_WINDOWS
            ReleaseHandle(entry);
#else
            (void)entry;
#endif
        });
}

} // namespace

bool UpsertRetainedParticleEmitter(
    const std::wstring& activeManifestPath,
    const ResolvedParticleEmitterCommand& resolved,
    std::string* outError) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)resolved;
    if (outError) {
        *outError = "retained particle emitters are not supported on this platform";
    }
    return false;
#else
    const std::wstring key = BuildRetainedEmitterKey(activeManifestPath, resolved.emitterId);
    if (key.empty()) {
        if (outError) {
            *outError = "retained particle emitter requires active manifest path and emitter id";
        }
        return false;
    }

    const ResolvedParticleEmitterCommand effectiveResolved = ResolveParticleEmitterForGroupTransform(
        ResolveParticleEmitterForGroupMaterial(
            resolved,
            ResolveGroupMaterial(activeManifestPath, resolved.semantics.groupId)),
        ResolveEffectiveGroupTransform(activeManifestPath, resolved.semantics.groupId, resolved.useGroupLocalOrigin));

    std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
    PruneInactiveRetainedEmittersLocked();
    auto& entries = RetainedEmitterStore::Entries();
    auto [it, inserted] = entries.try_emplace(key, RetainedEmitterEntry{});
    if (inserted
#if MFX_PLATFORM_MACOS
        || it->second.handle == nullptr
#elif MFX_PLATFORM_WINDOWS
        || it->second.rippleId == 0u
        || !it->second.state
#endif
    ) {
#if MFX_PLATFORM_MACOS
        it->second.handle = mfx_macos_wasm_retained_particle_emitter_create_v1();
        if (it->second.handle == nullptr) {
            entries.erase(it);
            if (outError) {
                *outError = "failed to create retained particle emitter handle";
            }
            return false;
        }
#elif MFX_PLATFORM_WINDOWS
        if (!CreateHandle(&it->second, effectiveResolved, outError)) {
            entries.erase(it);
            return false;
        }
#endif
    } else {
#if MFX_PLATFORM_WINDOWS
        if (it->second.squareSizePx != effectiveResolved.squareSizePx) {
            ReleaseHandle(&it->second);
            if (!CreateHandle(&it->second, effectiveResolved, outError)) {
                entries.erase(it);
                return false;
            }
        }
#endif
    }

#if MFX_PLATFORM_MACOS
    UpsertHandle(it->second.handle, effectiveResolved);
    it->second.groupId = resolved.semantics.groupId;
    it->second.blendMode = resolved.semantics.blendMode;
    it->second.sortKey = resolved.semantics.sortKey;
    it->second.clipRect = resolved.semantics.clipRect;
    it->second.useGroupLocalOrigin = resolved.useGroupLocalOrigin;
    it->second.sourceResolved = resolved;
    it->second.baseFrameLeftPx = effectiveResolved.frameLeftPx;
    it->second.baseFrameTopPx = effectiveResolved.frameTopPx;
    ApplyHandleGroupPresentation(
        it->second.handle,
        ResolveGroupPresentation(activeManifestPath, resolved.semantics.groupId));
    {
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, resolved.semantics.groupId);
        ApplyHandleGroupLayer(
            it->second.handle,
            ResolveEffectiveGroupBlendMode(it->second.blendMode, layerState),
            ResolveEffectiveGroupSortKey(it->second.sortKey, layerState));
    }
    const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, resolved.semantics.groupId);
    ApplyHandleGroupClipRect(
        it->second.handle,
        IntersectRenderClipRects(
            it->second.clipRect,
            groupClipState.clipRect),
        groupClipState.maskShapeKind,
        groupClipState.cornerRadiusPx);
    {
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, it->second);
        ApplyHandleGroupTransform(it->second.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
    }
#elif MFX_PLATFORM_WINDOWS
    UpsertHandle(&it->second, effectiveResolved);
    it->second.groupId = resolved.semantics.groupId;
    it->second.blendMode = resolved.semantics.blendMode;
    it->second.sortKey = resolved.semantics.sortKey;
    it->second.clipRect = resolved.semantics.clipRect;
    it->second.useGroupLocalOrigin = resolved.useGroupLocalOrigin;
    it->second.sourceResolved = resolved;
    it->second.baseFrameLeftPx = effectiveResolved.frameLeftPx;
    it->second.baseFrameTopPx = effectiveResolved.frameTopPx;
    it->second.baseCenterX = effectiveResolved.screenPt.x;
    it->second.baseCenterY = effectiveResolved.screenPt.y;
    ApplyHandleGroupPresentation(
        &it->second,
        ResolveGroupPresentation(activeManifestPath, resolved.semantics.groupId));
    {
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, resolved.semantics.groupId);
        ApplyHandleGroupLayer(
            &it->second,
            ResolveEffectiveGroupBlendMode(it->second.blendMode, layerState),
            ResolveEffectiveGroupSortKey(it->second.sortKey, layerState));
    }
    const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, resolved.semantics.groupId);
    ApplyHandleGroupClipRect(
        &it->second,
        IntersectRenderClipRects(
            it->second.clipRect,
            groupClipState.clipRect),
        groupClipState.maskShapeKind,
        groupClipState.cornerRadiusPx);
    {
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, it->second);
        ApplyHandleGroupTransform(&it->second, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
    }
#endif
    RetainedEmitterStore::UpsertRequests().fetch_add(1u, std::memory_order_relaxed);
    return true;
#endif
}

bool RemoveRetainedParticleEmitter(
    const std::wstring& activeManifestPath,
    uint32_t emitterId,
    std::string* outError) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)emitterId;
    if (outError) {
        *outError = "retained particle emitters are not supported on this platform";
    }
    return false;
#else
    const std::wstring key = BuildRetainedEmitterKey(activeManifestPath, emitterId);
    if (key.empty()) {
        if (outError) {
            *outError = "retained particle emitter remove requires active manifest path and emitter id";
        }
        return false;
    }

    void* handle = nullptr;
#if MFX_PLATFORM_WINDOWS
    RetainedEmitterEntry entry{};
#endif
    {
        std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
        auto& entries = RetainedEmitterStore::Entries();
        const auto it = entries.find(key);
        if (it != entries.end()) {
#if MFX_PLATFORM_MACOS
            handle = it->second.handle;
#elif MFX_PLATFORM_WINDOWS
            entry = std::move(it->second);
#endif
            entries.erase(it);
        }
    }

#if MFX_PLATFORM_MACOS
    ReleaseHandle(handle);
#elif MFX_PLATFORM_WINDOWS
    ReleaseHandle(&entry);
#endif
    RetainedEmitterStore::RemoveRequests().fetch_add(1u, std::memory_order_relaxed);
    return true;
#endif
}

uint32_t RemoveRetainedParticleEmittersByGroup(
    const std::wstring& activeManifestPath,
    uint32_t groupId) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    return 0u;
#else
    return RemoveRetainedEmittersForGroup<RetainedEmitterStore>(
        activeManifestPath,
        groupId,
        [](const RetainedEmitterEntry& entry, uint32_t candidateGroupId) {
            return entry.groupId == candidateGroupId;
        },
        [](RetainedEmitterEntry* entry) {
#if MFX_PLATFORM_MACOS
            ReleaseHandle(entry ? entry->handle : nullptr);
#elif MFX_PLATFORM_WINDOWS
            ReleaseHandle(entry);
#else
            (void)entry;
#endif
        });
#endif
}

void ApplyRetainedParticleEmitterGroupPresentation(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float alphaMultiplier,
    bool visible) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)alphaMultiplier;
    (void)visible;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }

    const GroupPresentationState presentation{
        std::clamp(alphaMultiplier, 0.0f, 1.0f),
        visible,
    };
    std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
    PruneInactiveRetainedEmittersLocked();
    for (auto& [key, entry] : RetainedEmitterStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
#if MFX_PLATFORM_MACOS
        ApplyHandleGroupPresentation(entry.handle, presentation);
#elif MFX_PLATFORM_WINDOWS
        ApplyHandleGroupPresentation(&entry, presentation);
#endif
    }
#endif
}

void ApplyRetainedParticleEmitterGroupClipRect(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    const mousefx::RenderClipRect& groupClipRect) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)groupClipRect;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }

    std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
    PruneInactiveRetainedEmittersLocked();
    for (auto& [key, entry] : RetainedEmitterStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, groupId);
#if MFX_PLATFORM_MACOS
        ApplyHandleGroupClipRect(
            entry.handle,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
#elif MFX_PLATFORM_WINDOWS
        ApplyHandleGroupClipRect(
            &entry,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
#endif
    }
#endif
}

void ApplyRetainedParticleEmitterGroupLayer(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasBlendOverride,
    mousefx::RenderBlendMode blendMode,
    int32_t sortBias) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)hasBlendOverride;
    (void)blendMode;
    (void)sortBias;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }

    const GroupLayerState layerState{hasBlendOverride, blendMode, sortBias};
    std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
    PruneInactiveRetainedEmittersLocked();
    for (auto& [key, entry] : RetainedEmitterStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
#if MFX_PLATFORM_MACOS
        ApplyHandleGroupLayer(
            entry.handle,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
#elif MFX_PLATFORM_WINDOWS
        ApplyHandleGroupLayer(
            &entry,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
#endif
    }
#endif
}

void ApplyRetainedParticleEmitterGroupTransform(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float offsetXPx,
    float offsetYPx,
    float rotationRad,
    float uniformScale) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)offsetXPx;
    (void)offsetYPx;
    (void)rotationRad;
    (void)uniformScale;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }
    (void)offsetXPx;
    (void)offsetYPx;
    (void)rotationRad;
    (void)uniformScale;

    std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
    PruneInactiveRetainedEmittersLocked();
    for (auto& [key, entry] : RetainedEmitterStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const GroupEffectiveTransform effectiveTransform = ResolveEffectiveGroupTransform(
            activeManifestPath,
            entry.groupId,
            entry.useGroupLocalOrigin);
        const ResolvedParticleEmitterCommand effectiveResolved =
            ResolveEntryParticleEmitterCommand(activeManifestPath, entry);
#if MFX_PLATFORM_MACOS
        UpsertHandle(entry.handle, effectiveResolved);
        entry.baseFrameLeftPx = effectiveResolved.frameLeftPx;
        entry.baseFrameTopPx = effectiveResolved.frameTopPx;
        ApplyHandleGroupTransform(entry.handle, effectiveTransform.offsetXPx, effectiveTransform.offsetYPx);
#elif MFX_PLATFORM_WINDOWS
        UpsertHandle(&entry, effectiveResolved);
        entry.baseFrameLeftPx = effectiveResolved.frameLeftPx;
        entry.baseFrameTopPx = effectiveResolved.frameTopPx;
        entry.baseCenterX = effectiveResolved.screenPt.x;
        entry.baseCenterY = effectiveResolved.screenPt.y;
        ApplyHandleGroupTransform(&entry, effectiveTransform.offsetXPx, effectiveTransform.offsetYPx);
#endif
    }
#endif
}

void ApplyRetainedParticleEmitterGroupLocalOrigin(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    float originXPx,
    float originYPx) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)originXPx;
    (void)originYPx;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }
    (void)originXPx;
    (void)originYPx;

    std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
    PruneInactiveRetainedEmittersLocked();
    for (auto& [key, entry] : RetainedEmitterStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
#if MFX_PLATFORM_MACOS
        ApplyHandleGroupTransform(entry.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#elif MFX_PLATFORM_WINDOWS
        ApplyHandleGroupTransform(&entry, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#endif
    }
#endif
}

void ApplyRetainedParticleEmitterGroupMaterial(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    bool hasTintOverride,
    uint32_t tintArgb,
    float intensityMultiplier,
    uint8_t styleKind,
    float styleAmount,
    float diffusionAmount,
    float persistenceAmount,
    float echoAmount,
    float echoDriftPx,
    uint8_t feedbackMode,
    float feedbackPhaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)hasTintOverride;
    (void)tintArgb;
    (void)intensityMultiplier;
    (void)styleKind;
    (void)styleAmount;
    (void)diffusionAmount;
    (void)persistenceAmount;
    (void)echoAmount;
    (void)echoDriftPx;
    (void)feedbackMode;
    (void)feedbackPhaseRad;
    (void)feedbackLayerCount;
    (void)feedbackLayerFalloff;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }
    (void)hasTintOverride;
    (void)tintArgb;
    (void)intensityMultiplier;
    (void)styleKind;
    (void)styleAmount;
    (void)diffusionAmount;
    (void)persistenceAmount;
    (void)echoAmount;
    (void)echoDriftPx;
    (void)feedbackMode;
    (void)feedbackPhaseRad;

    std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
    PruneInactiveRetainedEmittersLocked();
    for (auto& [key, entry] : RetainedEmitterStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const ResolvedParticleEmitterCommand effectiveResolved = ResolveEntryParticleEmitterCommand(activeManifestPath, entry);
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, groupId);
        const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, groupId);
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
#if MFX_PLATFORM_MACOS
        UpsertHandle(entry.handle, effectiveResolved);
        ApplyHandleGroupLayer(
            entry.handle,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        ApplyHandleGroupClipRect(
            entry.handle,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        ApplyHandleGroupTransform(entry.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#elif MFX_PLATFORM_WINDOWS
        UpsertHandle(&entry, effectiveResolved);
        ApplyHandleGroupLayer(
            &entry,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        ApplyHandleGroupClipRect(
            &entry,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        ApplyHandleGroupTransform(&entry, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#endif
    }
#endif
}

void ApplyRetainedParticleEmitterGroupPass(
    const std::wstring& activeManifestPath,
    uint32_t groupId,
    uint8_t passKind,
    float passAmount,
    float responseAmount,
    uint8_t passMode,
    float phaseRad,
    uint8_t feedbackLayerCount,
    float feedbackLayerFalloff) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)groupId;
    (void)passKind;
    (void)passAmount;
    (void)responseAmount;
    (void)passMode;
    (void)phaseRad;
    (void)feedbackLayerCount;
    (void)feedbackLayerFalloff;
    return;
#else
    if (activeManifestPath.empty() || groupId == 0u) {
        return;
    }
    (void)passKind;
    (void)passAmount;
    (void)responseAmount;
    (void)passMode;
    (void)phaseRad;
    (void)feedbackLayerCount;
    (void)feedbackLayerFalloff;

    std::lock_guard<std::mutex> lock(RetainedEmitterStore::Mutex());
    PruneInactiveRetainedEmittersLocked();
    for (auto& [key, entry] : RetainedEmitterStore::Entries()) {
        if (key.rfind(activeManifestPath + L"#", 0) != 0 || entry.groupId != groupId) {
            continue;
        }
        const ResolvedParticleEmitterCommand effectiveResolved = ResolveEntryParticleEmitterCommand(activeManifestPath, entry);
        const GroupLayerState layerState = ResolveGroupLayer(activeManifestPath, groupId);
        const auto groupClipState = ResolveGroupClipRectState(activeManifestPath, groupId);
        const GroupEffectiveOffset effectiveOffset = ResolveEntryGroupOffset(activeManifestPath, entry);
#if MFX_PLATFORM_MACOS
        UpsertHandle(entry.handle, effectiveResolved);
        ApplyHandleGroupLayer(
            entry.handle,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        ApplyHandleGroupClipRect(
            entry.handle,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        ApplyHandleGroupTransform(entry.handle, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#elif MFX_PLATFORM_WINDOWS
        UpsertHandle(&entry, effectiveResolved);
        ApplyHandleGroupLayer(
            &entry,
            ResolveEffectiveGroupBlendMode(entry.blendMode, layerState),
            ResolveEffectiveGroupSortKey(entry.sortKey, layerState));
        ApplyHandleGroupClipRect(
            &entry,
            IntersectRenderClipRects(entry.clipRect, groupClipState.clipRect),
            groupClipState.maskShapeKind,
            groupClipState.cornerRadiusPx);
        ApplyHandleGroupTransform(&entry, effectiveOffset.offsetXPx, effectiveOffset.offsetYPx);
#endif
    }
#endif
}

void ResetRetainedParticleEmittersForManifest(const std::wstring& activeManifestPath) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    return;
#else
    if (activeManifestPath.empty()) {
        return;
    }

    ResetRetainedEmittersForManifest<RetainedEmitterStore>(
        activeManifestPath,
        [](RetainedEmitterEntry* entry) {
#if MFX_PLATFORM_MACOS
            ReleaseHandle(entry ? entry->handle : nullptr);
#elif MFX_PLATFORM_WINDOWS
            ReleaseHandle(entry);
#else
            (void)entry;
#endif
        });
#endif
}

void ResetAllRetainedParticleEmitters() {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    return;
#else
    ResetAllRetainedEmitters<RetainedEmitterStore>(
        [](RetainedEmitterEntry* entry) {
#if MFX_PLATFORM_MACOS
            ReleaseHandle(entry ? entry->handle : nullptr);
#elif MFX_PLATFORM_WINDOWS
            ReleaseHandle(entry);
#else
            (void)entry;
#endif
        });
#endif
}

RetainedParticleEmitterRuntimeCounters GetRetainedParticleEmitterRuntimeCounters() {
    RetainedParticleEmitterRuntimeCounters counters{};
    counters.upsertRequests = RetainedEmitterStore::UpsertRequests().load(std::memory_order_relaxed);
    counters.removeRequests = RetainedEmitterStore::RemoveRequests().load(std::memory_order_relaxed);
#if MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS
    counters.activeEmitters = CountActiveRetainedEmitters<RetainedEmitterStore>(
        [](const RetainedEmitterEntry& entry) {
#if MFX_PLATFORM_MACOS
            return IsHandleActive(entry.handle);
#elif MFX_PLATFORM_WINDOWS
            return IsHandleActive(entry);
#else
            return true;
#endif
        },
        [](RetainedEmitterEntry* entry) {
#if MFX_PLATFORM_MACOS
            ReleaseHandle(entry ? entry->handle : nullptr);
#elif MFX_PLATFORM_WINDOWS
            ReleaseHandle(entry);
#else
            (void)entry;
#endif
        });
#endif
    return counters;
}

} // namespace mousefx::wasm
