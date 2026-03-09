#include "pch.h"

#include "MouseFx/Core/Wasm/WasmRetainedSpriteEmitterRuntime.h"
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

#include "MouseFx/Utils/StringUtils.h"

#if MFX_PLATFORM_MACOS
#include "Platform/macos/Wasm/MacosWasmRetainedSpriteEmitterSwiftBridge.h"
#endif

#include <cmath>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

namespace mousefx::wasm {

namespace {

struct RetainedEmitterEntry final {
#if MFX_PLATFORM_MACOS
    void* handle = nullptr;
#elif MFX_PLATFORM_WINDOWS
    uint64_t rippleId = 0u;
    int squareSizePx = 0;
    std::shared_ptr<class WindowsRetainedSpriteEmitterSharedState> state{};
#endif
    uint32_t groupId = 0u;
    mousefx::RenderBlendMode blendMode = mousefx::RenderBlendMode::Normal;
    int32_t sortKey = 0;
    mousefx::RenderClipRect clipRect{};
    bool useGroupLocalOrigin = false;
    ResolvedSpriteEmitterCommand sourceResolved{};
    int32_t baseFrameLeftPx = 0;
    int32_t baseFrameTopPx = 0;
#if MFX_PLATFORM_WINDOWS
    int32_t baseCenterX = 0;
    int32_t baseCenterY = 0;
#endif
};

using RetainedEmitterStore = RetainedEmitterRuntimeStore<RetainedEmitterEntry>;

ResolvedSpriteEmitterCommand ResolveSpriteEmitterForGroupTransform(
    const ResolvedSpriteEmitterCommand& source,
    const GroupEffectiveTransform& transform) {
    if (!HasGeometryGroupTransform(source.useGroupLocalOrigin, transform.rotationRad, transform.scaleX, transform.scaleY)) {
        return source;
    }

    ResolvedSpriteEmitterCommand resolved = source;
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

    resolved.directionRad = NormalizeSpriteEmitterRadians(
        std::atan2(transformedDirection.y, transformedDirection.x),
        source.directionRad);
    resolved.speedMin = ClampSpriteEmitterFloat(source.speedMin * directionScale, source.speedMin, 1.0f, 2400.0f);
    resolved.speedMax = ClampSpriteEmitterFloat(source.speedMax * directionScale, source.speedMax, resolved.speedMin, 3200.0f);
    resolved.sizeMinPx = ClampSpriteEmitterFloat(source.sizeMinPx * scale, source.sizeMinPx, 4.0f, 220.0f);
    resolved.sizeMaxPx = ClampSpriteEmitterFloat(source.sizeMaxPx * scale, source.sizeMaxPx, resolved.sizeMinPx, 320.0f);
    resolved.rotationMinRad = NormalizeSpriteEmitterRadians(source.rotationMinRad + transform.rotationRad, source.rotationMinRad);
    resolved.rotationMaxRad = NormalizeSpriteEmitterRadians(source.rotationMaxRad + transform.rotationRad, source.rotationMaxRad);
    if (resolved.rotationMaxRad < resolved.rotationMinRad) {
        std::swap(resolved.rotationMinRad, resolved.rotationMaxRad);
    }
    resolved.accelerationX = ClampSpriteEmitterFloat(transformedAcceleration.x, source.accelerationX, -4800.0f, 4800.0f);
    resolved.accelerationY = ClampSpriteEmitterFloat(transformedAcceleration.y, source.accelerationY, -4800.0f, 4800.0f);

    const ScreenPoint screenPoint = ClampSpriteEmitterScreenPoint(transformedCenter.x, transformedCenter.y);
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
    return resolved;
}

ResolvedSpriteEmitterCommand ResolveSpriteEmitterForGroupMaterial(
    const ResolvedSpriteEmitterCommand& source,
    const GroupMaterialState& materialState) {
    ResolvedSpriteEmitterCommand resolved = source;
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
    resolved.applyTint = GroupMaterialChangesSpriteTint(source.applyTint, materialState);
    resolved.tintArgb = ApplyGroupMaterialToSpriteTint(source.tintArgb, source.applyTint, materialState);
    resolved.sizeMinPx = ClampSpriteEmitterFloat(
        source.sizeMinPx * styleProfile.sizeMultiplier,
        source.sizeMinPx,
        4.0f,
        220.0f);
    resolved.sizeMaxPx = ClampSpriteEmitterFloat(
        source.sizeMaxPx * styleProfile.sizeMultiplier,
        source.sizeMaxPx,
        resolved.sizeMinPx,
        320.0f);
    resolved.alphaMin = ClampSpriteEmitterFloat(
        source.alphaMin * styleProfile.alphaMultiplier,
        source.alphaMin,
        0.01f,
        1.0f);
    resolved.alphaMax = ClampSpriteEmitterFloat(
        source.alphaMax * styleProfile.alphaMultiplier,
        source.alphaMax,
        resolved.alphaMin,
        1.0f);
    resolved.spreadRad = ClampSpriteEmitterFloat(
        source.spreadRad * styleProfile.spreadMultiplier,
        source.spreadRad,
        0.0f,
        6.2831853f);
    resolved.rotationMinRad = NormalizeSpriteEmitterRadians(
        source.rotationMinRad - styleProfile.spreadMultiplier * 0.08f,
        source.rotationMinRad);
    resolved.rotationMaxRad = NormalizeSpriteEmitterRadians(
        source.rotationMaxRad + styleProfile.spreadMultiplier * 0.08f,
        source.rotationMaxRad);
    if (resolved.rotationMaxRad < resolved.rotationMinRad) {
        std::swap(resolved.rotationMinRad, resolved.rotationMaxRad);
    }
    resolved.emitterTtlMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.emitterTtlMs) * styleProfile.ttlMultiplier)),
        40u,
        10000u);
    resolved.particleLifeMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.particleLifeMs) * styleProfile.lifeMultiplier)),
        60u,
        12000u);
    resolved.screenPt = ClampSpriteEmitterScreenPoint(
        static_cast<float>(source.screenPt.x) + echoDrift.x,
        static_cast<float>(source.screenPt.y) + echoDrift.y);
    {
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(resolved.screenPt);
        resolved.frameLeftPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.x) - resolved.squareSizePx * 0.5));
        resolved.frameTopPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.y) - resolved.squareSizePx * 0.5));
        resolved.localX = static_cast<float>(overlayPoint.x - resolved.frameLeftPx);
        resolved.localY = static_cast<float>(overlayPoint.y - resolved.frameTopPx);
    }
    return resolved;
}

ResolvedSpriteEmitterCommand ResolveSpriteEmitterForGroupPass(
    const ResolvedSpriteEmitterCommand& source,
    const GroupPassState& passState) {
    ResolvedSpriteEmitterCommand resolved = source;
    const GroupPassStyleProfile passProfile = ResolveGroupPassStyleProfileForLane(
        passState,
        kGroupPassRouteSprite);
    const GroupPassEchoVector echoDrift = ResolveGroupPassEchoDrift(
        source.accelerationX,
        source.accelerationY,
        source.directionRad,
        passProfile.echoDriftPx,
        passProfile.passMode,
        passProfile.phaseRad,
        passProfile.feedbackLayerCount,
        passProfile.feedbackLayerFalloff);
    resolved.sizeMinPx = ClampSpriteEmitterFloat(
        source.sizeMinPx * passProfile.sizeMultiplier,
        source.sizeMinPx,
        4.0f,
        220.0f);
    resolved.sizeMaxPx = ClampSpriteEmitterFloat(
        source.sizeMaxPx * passProfile.sizeMultiplier,
        source.sizeMaxPx,
        resolved.sizeMinPx,
        320.0f);
    resolved.alphaMin = ClampSpriteEmitterFloat(
        source.alphaMin * passProfile.alphaMultiplier,
        source.alphaMin,
        0.01f,
        1.0f);
    resolved.alphaMax = ClampSpriteEmitterFloat(
        source.alphaMax * passProfile.alphaMultiplier,
        source.alphaMax,
        resolved.alphaMin,
        1.0f);
    resolved.spreadRad = ClampSpriteEmitterFloat(
        source.spreadRad * passProfile.spreadMultiplier,
        source.spreadRad,
        0.0f,
        6.2831853f);
    resolved.emitterTtlMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.emitterTtlMs) * passProfile.ttlMultiplier)),
        40u,
        10000u);
    resolved.particleLifeMs = std::clamp<uint32_t>(
        static_cast<uint32_t>(std::lround(static_cast<double>(source.particleLifeMs) * passProfile.lifeMultiplier)),
        60u,
        12000u);
    resolved.screenPt = ClampSpriteEmitterScreenPoint(
        static_cast<float>(source.screenPt.x) + echoDrift.x,
        static_cast<float>(source.screenPt.y) + echoDrift.y);
    {
        const ScreenPoint overlayPoint = ScreenToOverlayPoint(resolved.screenPt);
        resolved.frameLeftPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.x) - resolved.squareSizePx * 0.5));
        resolved.frameTopPx = static_cast<int>(std::floor(static_cast<double>(overlayPoint.y) - resolved.squareSizePx * 0.5));
        resolved.localX = static_cast<float>(overlayPoint.x - resolved.frameLeftPx);
        resolved.localY = static_cast<float>(overlayPoint.y - resolved.frameTopPx);
    }
    return resolved;
}

ResolvedSpriteEmitterCommand ResolveEntrySpriteEmitterCommand(
    const std::wstring& activeManifestPath,
    const RetainedEmitterEntry& entry) {
    return ResolveSpriteEmitterForGroupTransform(
        ResolveSpriteEmitterForGroupPass(
            ResolveSpriteEmitterForGroupMaterial(
                entry.sourceResolved,
                ResolveGroupMaterial(activeManifestPath, entry.groupId)),
            ResolveGroupPass(activeManifestPath, entry.groupId)),
        ResolveEffectiveGroupTransform(activeManifestPath, entry.groupId, entry.useGroupLocalOrigin));
}

#if MFX_PLATFORM_MACOS
bool IsHandleActive(void* handle) {
    return handle != nullptr && mfx_macos_wasm_retained_sprite_emitter_is_active_v1(handle) != 0;
}

void ReleaseHandle(void* handle) {
    if (handle != nullptr) {
        mfx_macos_wasm_retained_sprite_emitter_release_v1(handle);
    }
}

void UpsertHandle(void* handle, const ResolvedSpriteEmitterCommand& resolved) {
    const std::string imagePathUtf8 = resolved.assetPath.empty()
        ? std::string{}
        : Utf16ToUtf8(resolved.assetPath.c_str());
    mfx_macos_wasm_retained_sprite_emitter_upsert_v1(
        handle,
        resolved.frameLeftPx,
        resolved.frameTopPx,
        resolved.squareSizePx,
        resolved.localX,
        resolved.localY,
        imagePathUtf8.empty() ? nullptr : imagePathUtf8.c_str(),
        resolved.emissionRatePerSec,
        resolved.directionRad,
        resolved.spreadRad,
        resolved.speedMin,
        resolved.speedMax,
        resolved.sizeMinPx,
        resolved.sizeMaxPx,
        resolved.alphaMin,
        resolved.alphaMax,
        resolved.tintArgb,
        resolved.applyTint ? 1u : 0u,
        resolved.rotationMinRad,
        resolved.rotationMaxRad,
        resolved.accelerationX,
        resolved.accelerationY,
        resolved.emitterTtlMs,
        resolved.particleLifeMs,
        resolved.maxParticles,
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
    mfx_macos_wasm_retained_sprite_emitter_set_group_presentation_v1(
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
    mfx_macos_wasm_retained_sprite_emitter_set_effective_clip_rect_v2(
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
    mfx_macos_wasm_retained_sprite_emitter_set_effective_layer_v1(
        handle,
        static_cast<uint32_t>(blendMode),
        sortKey);
}

void ApplyHandleGroupTransform(void* handle, float offsetXPx, float offsetYPx) {
    if (handle == nullptr) {
        return;
    }
    mfx_macos_wasm_retained_sprite_emitter_set_effective_translation_v1(handle, offsetXPx, offsetYPx);
}
#elif MFX_PLATFORM_WINDOWS
class WindowsRetainedSpriteEmitterSharedState final {
public:
    void Upsert(const ResolvedSpriteEmitterCommand& resolved) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = resolved;
        stopRequested_ = false;
        alive_ = true;
        expireTickMs_ = NowMs() + static_cast<uint64_t>(resolved.emitterTtlMs);
        if (rngState_ == 0u) {
            rngState_ = resolved.emitterId ^ 0x9E3779B9u;
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
        std::vector<DrawSprite> drawSprites;
        RenderBlendMode blendMode = RenderBlendMode::Normal;
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
                particle.x += particle.vx * dtSec + 0.5f * config_.accelerationX * dtSec * dtSec;
                particle.y += particle.vy * dtSec + 0.5f * config_.accelerationY * dtSec * dtSec;
                particle.vx += config_.accelerationX * dtSec;
                particle.vy += config_.accelerationY * dtSec;
                particle.ageSec += dtSec;
                if (particle.ageSec >= particle.lifeSec) {
                    continue;
                }
                updated.push_back(std::move(particle));
            }
            particles_.swap(updated);

            if ((stopRequested_ || nowMs >= expireTickMs_) && particles_.empty()) {
                alive_ = false;
                return;
            }

            drawSprites.reserve(particles_.size());
            for (const Particle& particle : particles_) {
                const float life = std::max(0.0f, 1.0f - particle.ageSec / std::max(0.01f, particle.lifeSec));
                const float alpha = particle.alpha * std::pow(life, 0.62f);
                if (alpha <= 0.001f || particle.sizePx <= 0.5f) {
                    continue;
                }
                drawSprites.push_back(DrawSprite{
                    particle.x,
                    particle.y,
                    particle.sizePx,
                    alpha,
                    particle.rotationRad,
                    particle.tintArgb,
                    particle.applyTint,
                    particle.assetPath,
                });
            }
        }

        if (drawSprites.empty()) {
            return;
        }

        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        const bool screenBlend = UsesScreenLikeBlend(blendMode);
        for (const DrawSprite& sprite : drawSprites) {
            const float sizePx = std::max(2.0f, sprite.sizePx);
            const float drawX = sprite.x - sizePx * 0.5f;
            const float drawY = sprite.y - sizePx * 0.5f;
            Gdiplus::Image* image = EnsureImageLoaded(sprite.assetPath);
            if (image != nullptr) {
                DrawImageSprite(g, sprite, image, drawX, drawY, sizePx);
            } else {
                DrawFallbackSprite(g, sprite, drawX, drawY, sizePx, screenBlend);
            }
        }
    }

private:
    struct DrawSprite final {
        float x = 0.0f;
        float y = 0.0f;
        float sizePx = 0.0f;
        float alpha = 0.0f;
        float rotationRad = 0.0f;
        uint32_t tintArgb = 0xFFFFFFFFu;
        bool applyTint = false;
        std::wstring assetPath{};
    };

    struct Particle final {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float sizePx = 24.0f;
        float alpha = 1.0f;
        float rotationRad = 0.0f;
        uint32_t tintArgb = 0xFFFFFFFFu;
        bool applyTint = false;
        std::wstring assetPath{};
        float ageSec = 0.0f;
        float lifeSec = 1.0f;
    };

    struct BitmapCacheEntry final {
        std::wstring path{};
        std::unique_ptr<Gdiplus::Image> image{};
        bool attempted = false;
    };

    static constexpr float kRadToDeg = 57.29577951308232f;

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
        const float halfSpread = config_.spreadRad * 0.5f;
        const float angle = config_.directionRad + RandomLocked(-halfSpread, halfSpread);
        const float speed = RandomLocked(config_.speedMin, config_.speedMax);
        Particle particle{};
        particle.x = config_.localX;
        particle.y = config_.localY;
        particle.vx = std::cos(angle) * speed;
        particle.vy = std::sin(angle) * speed;
        particle.sizePx = RandomLocked(config_.sizeMinPx, config_.sizeMaxPx);
        particle.alpha = RandomLocked(config_.alphaMin, config_.alphaMax);
        particle.rotationRad = RandomLocked(config_.rotationMinRad, config_.rotationMaxRad);
        particle.tintArgb = config_.tintArgb;
        particle.applyTint = config_.applyTint;
        particle.assetPath = config_.assetPath;
        particle.lifeSec = std::max(0.06f, static_cast<float>(config_.particleLifeMs) / 1000.0f);
        return particle;
    }

    Gdiplus::Image* EnsureImageLoaded(const std::wstring& path) {
        if (path.empty()) {
            return nullptr;
        }

        for (BitmapCacheEntry& entry : cache_) {
            if (entry.path != path) {
                continue;
            }
            if (!entry.attempted || !entry.image) {
                return nullptr;
            }
            return entry.image.get();
        }

        BitmapCacheEntry entry{};
        entry.path = path;
        entry.attempted = true;
        std::unique_ptr<Gdiplus::Image> image(Gdiplus::Image::FromFile(path.c_str(), FALSE));
        if (image && image->GetLastStatus() == Gdiplus::Ok) {
            entry.image = std::move(image);
        }
        cache_.push_back(std::move(entry));
        return cache_.back().image.get();
    }

    static float Clamp01(float value) {
        return std::clamp(value, 0.0f, 1.0f);
    }

    void ApplyTintColorMatrix(
        uint32_t tintArgb,
        bool applyTint,
        float alphaScale,
        float outMatrix[5][5]) const {
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 5; ++col) {
                outMatrix[row][col] = (row == col) ? 1.0f : 0.0f;
            }
        }

        const float effectiveAlpha = Clamp01(alphaScale);
        outMatrix[3][3] = effectiveAlpha;
        if (!applyTint) {
            return;
        }

        const float tintA = static_cast<float>((tintArgb >> 24) & 0xFFu) / 255.0f;
        const float tintR = static_cast<float>((tintArgb >> 16) & 0xFFu) / 255.0f;
        const float tintG = static_cast<float>((tintArgb >> 8) & 0xFFu) / 255.0f;
        const float tintB = static_cast<float>(tintArgb & 0xFFu) / 255.0f;
        outMatrix[0][0] = tintR;
        outMatrix[1][1] = tintG;
        outMatrix[2][2] = tintB;
        outMatrix[3][3] = Clamp01(effectiveAlpha * tintA);
    }

    void DrawImageSprite(
        Gdiplus::Graphics& g,
        const DrawSprite& sprite,
        Gdiplus::Image* image,
        float drawX,
        float drawY,
        float sizePx) {
        const float imageW = static_cast<float>(image->GetWidth());
        const float imageH = static_cast<float>(image->GetHeight());
        if (imageW <= 0.0f || imageH <= 0.0f) {
            DrawFallbackSprite(g, sprite, drawX, drawY, sizePx, false);
            return;
        }

        const float fitScale = std::min(sizePx / imageW, sizePx / imageH);
        const float drawW = std::max(1.0f, imageW * fitScale);
        const float drawH = std::max(1.0f, imageH * fitScale);
        const float localDrawX = (sizePx - drawW) * 0.5f;
        const float localDrawY = (sizePx - drawH) * 0.5f;

        float matrixData[5][5] = {};
        ApplyTintColorMatrix(sprite.tintArgb, sprite.applyTint, sprite.alpha, matrixData);
        Gdiplus::ColorMatrix matrix;
        std::memcpy(&matrix, matrixData, sizeof(matrix));
        Gdiplus::ImageAttributes attrs;
        attrs.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

        const Gdiplus::GraphicsState state = g.Save();
        g.TranslateTransform(drawX + sizePx * 0.5f, drawY + sizePx * 0.5f);
        if (std::abs(sprite.rotationRad) > 0.001f) {
            g.RotateTransform(sprite.rotationRad * kRadToDeg);
        }
        g.TranslateTransform(-sizePx * 0.5f, -sizePx * 0.5f);
        g.DrawImage(
            image,
            Gdiplus::RectF(localDrawX, localDrawY, drawW, drawH),
            0.0f,
            0.0f,
            imageW,
            imageH,
            Gdiplus::UnitPixel,
            &attrs);
        g.Restore(state);
    }

    void DrawFallbackSprite(
        Gdiplus::Graphics& g,
        const DrawSprite& sprite,
        float drawX,
        float drawY,
        float sizePx,
        bool screenBlend) const {
        const float radius = sizePx * 0.5f;
        const Gdiplus::Color base = render_utils::ToGdiPlus({sprite.tintArgb});
        const BYTE coreAlpha = render_utils::ClampByte(
            static_cast<int>(static_cast<float>(base.GetA()) * sprite.alpha));
        if (coreAlpha == 0u) {
            return;
        }

        const float glowRadius = radius * (screenBlend ? 1.95f : 1.65f);
        Gdiplus::GraphicsPath path;
        path.AddEllipse(drawX + radius - glowRadius, drawY + radius - glowRadius, glowRadius * 2.0f, glowRadius * 2.0f);
        Gdiplus::PathGradientBrush glowBrush(&path);
        const Gdiplus::Color centerColor(
            render_utils::ClampByte(static_cast<int>(coreAlpha * (screenBlend ? 0.68f : 0.46f))),
            base.GetR(),
            base.GetG(),
            base.GetB());
        const Gdiplus::Color surroundColor(0u, base.GetR(), base.GetG(), base.GetB());
        INT count = 1;
        glowBrush.SetCenterColor(centerColor);
        glowBrush.SetSurroundColors(const_cast<Gdiplus::Color*>(&surroundColor), &count);
        g.FillPath(&glowBrush, &path);

        const Gdiplus::GraphicsState state = g.Save();
        g.TranslateTransform(drawX + radius, drawY + radius);
        if (std::abs(sprite.rotationRad) > 0.001f) {
            g.RotateTransform(sprite.rotationRad * kRadToDeg);
        }
        g.TranslateTransform(-radius, -radius);
        Gdiplus::SolidBrush coreBrush(Gdiplus::Color(coreAlpha, base.GetR(), base.GetG(), base.GetB()));
        g.FillEllipse(&coreBrush, 0.0f, 0.0f, sizePx, sizePx);
        g.Restore(state);
    }

    mutable std::mutex mutex_{};
    ResolvedSpriteEmitterCommand config_{};
    std::vector<Particle> particles_{};
    std::vector<BitmapCacheEntry> cache_{};
    float emitAccumulator_ = 0.0f;
    uint64_t lastTickMs_ = 0u;
    uint64_t expireTickMs_ = 0u;
    uint32_t rngState_ = 0x9E3779B9u;
    bool stopRequested_ = false;
    bool alive_ = true;
};

class WindowsRetainedSpriteEmitterRenderer final : public IRippleRenderer {
public:
    explicit WindowsRetainedSpriteEmitterRenderer(std::shared_ptr<WindowsRetainedSpriteEmitterSharedState> state)
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
    std::shared_ptr<WindowsRetainedSpriteEmitterSharedState> state_{};
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
    const ResolvedSpriteEmitterCommand& resolved,
    std::string* outError) {
    if (!entry) {
        if (outError) {
            *outError = "retained sprite emitter entry is null";
        }
        return false;
    }

    auto state = std::make_shared<WindowsRetainedSpriteEmitterSharedState>();
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
        std::make_unique<WindowsRetainedSpriteEmitterRenderer>(state),
        params);
    if (rippleId == 0u) {
        if (outError) {
            *outError = "failed to create retained sprite emitter overlay";
        }
        return false;
    }

    entry->rippleId = rippleId;
    entry->squareSizePx = resolved.squareSizePx;
    entry->state = std::move(state);
    entry->groupId = resolved.semantics.groupId;
    return true;
}

void UpsertHandle(RetainedEmitterEntry* entry, const ResolvedSpriteEmitterCommand& resolved) {
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

bool UpsertRetainedSpriteEmitter(
    const std::wstring& activeManifestPath,
    const ResolvedSpriteEmitterCommand& resolved,
    std::string* outError) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)resolved;
    if (outError) {
        *outError = "retained sprite emitters are not supported on this platform";
    }
    return false;
#else
    const std::wstring key = BuildRetainedEmitterKey(activeManifestPath, resolved.emitterId);
    if (key.empty()) {
        if (outError) {
            *outError = "retained sprite emitter requires active manifest path and emitter id";
        }
        return false;
    }

    const ResolvedSpriteEmitterCommand effectiveResolved = ResolveSpriteEmitterForGroupTransform(
        ResolveSpriteEmitterForGroupMaterial(
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
        it->second.handle = mfx_macos_wasm_retained_sprite_emitter_create_v1();
        if (it->second.handle == nullptr) {
            entries.erase(it);
            if (outError) {
                *outError = "failed to create retained sprite emitter handle";
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

bool RemoveRetainedSpriteEmitter(
    const std::wstring& activeManifestPath,
    uint32_t emitterId,
    std::string* outError) {
#if !(MFX_PLATFORM_MACOS || MFX_PLATFORM_WINDOWS)
    (void)activeManifestPath;
    (void)emitterId;
    if (outError) {
        *outError = "retained sprite emitters are not supported on this platform";
    }
    return false;
#else
    const std::wstring key = BuildRetainedEmitterKey(activeManifestPath, emitterId);
    if (key.empty()) {
        if (outError) {
            *outError = "retained sprite emitter remove requires active manifest path and emitter id";
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

uint32_t RemoveRetainedSpriteEmittersByGroup(
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

void ApplyRetainedSpriteEmitterGroupPresentation(
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

void ApplyRetainedSpriteEmitterGroupClipRect(
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

void ApplyRetainedSpriteEmitterGroupLayer(
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

void ApplyRetainedSpriteEmitterGroupTransform(
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
        const ResolvedSpriteEmitterCommand effectiveResolved = ResolveEntrySpriteEmitterCommand(activeManifestPath, entry);
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

void ApplyRetainedSpriteEmitterGroupLocalOrigin(
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

void ApplyRetainedSpriteEmitterGroupMaterial(
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
        const ResolvedSpriteEmitterCommand effectiveResolved = ResolveEntrySpriteEmitterCommand(activeManifestPath, entry);
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

void ApplyRetainedSpriteEmitterGroupPass(
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
        const ResolvedSpriteEmitterCommand effectiveResolved = ResolveEntrySpriteEmitterCommand(activeManifestPath, entry);
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

void ResetRetainedSpriteEmittersForManifest(const std::wstring& activeManifestPath) {
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

void ResetAllRetainedSpriteEmitters() {
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

RetainedSpriteEmitterRuntimeCounters GetRetainedSpriteEmitterRuntimeCounters() {
    RetainedSpriteEmitterRuntimeCounters counters{};
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
