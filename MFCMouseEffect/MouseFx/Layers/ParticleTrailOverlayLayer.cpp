#include "pch.h"

#include "ParticleTrailOverlayLayer.h"
#include "MouseFx/Core/Effects/TrailStyleCompute.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Utils/TimeUtils.h"

#include <algorithm>
#include <cmath>

namespace mousefx {

ParticleTrailOverlayLayer::ParticleTrailOverlayLayer(bool isChromatic) : isChromatic_(isChromatic) {
    rngState_ = static_cast<uint32_t>(NowMs() & 0xFFFFFFFFu);
    if (rngState_ == 0u) {
        rngState_ = 0x7F4A7C15u;
    }
}

void ParticleTrailOverlayLayer::AddCommand(const TrailEffectRenderCommand& command) {
    if (command.normalizedType == "none") {
        Clear();
        return;
    }
    if (!command.emit || command.normalizedType != "particle") {
        return;
    }

    const int emitCount = trail_style_compute::ComputeParticleEmitCount(std::max(1.0, command.speedPx));
    if (emitCount <= 0) {
        return;
    }
    Emit(command, emitCount);
}

void ParticleTrailOverlayLayer::Clear() {
    particles_.clear();
}

void ParticleTrailOverlayLayer::Update(uint64_t nowMs) {
    float dt = 0.016f;
    if (lastTickMs_ != 0 && nowMs >= lastTickMs_) {
        dt = (float)(nowMs - lastTickMs_) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
    }
    lastTickMs_ = nowMs;

    for (auto it = particles_.begin(); it != particles_.end();) {
        const auto step = trail_style_compute::ComputeParticleStepMetrics(
            it->x,
            it->y,
            it->vx,
            it->vy,
            it->life,
            it->size,
            dt * it->decayScale);
        it->x = static_cast<float>(step.nextX);
        it->y = static_cast<float>(step.nextY);
        it->vx = static_cast<float>(step.nextVx);
        it->vy = static_cast<float>(step.nextVy);
        it->life = static_cast<float>(step.nextLife);
        it->renderRadiusPx = static_cast<float>(step.renderRadiusPx);
        it->renderOpacity = static_cast<float>(step.renderOpacity);
        if (it->life <= 0.0f) {
            it = particles_.erase(it);
        } else {
            ++it;
        }
    }
}

void ParticleTrailOverlayLayer::Render(Gdiplus::Graphics& graphics) {
    if (particles_.empty()) return;
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    for (const auto& particle : particles_) {
        ScreenPoint screenPt{};
        screenPt.x = static_cast<int32_t>(std::lround(particle.x));
        screenPt.y = static_cast<int32_t>(std::lround(particle.y));
        const ScreenPoint localPt = ScreenToOverlayPoint(screenPt);

        BYTE alpha = static_cast<BYTE>(std::clamp<int>(
            static_cast<int>(std::lround(particle.renderOpacity * 255.0f)),
            0,
            255));
        Gdiplus::Color color = particle.useHue
            ? HslToRgb(particle.hue, 0.8f, 0.6f, alpha)
            : ArgbWithOpacity(particle.baseArgb, particle.renderOpacity);
        Gdiplus::SolidBrush brush(color);
        float size = std::max(0.0f, particle.renderRadiusPx * 2.0f);
        graphics.FillEllipse(&brush, (float)localPt.x - size * 0.5f, (float)localPt.y - size * 0.5f, size, size);
    }
}

Gdiplus::Color ParticleTrailOverlayLayer::HslToRgb(float h, float s, float l, BYTE alpha) {
    float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c * 0.5f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    if (h < 60.0f) {
        r = c; g = x;
    } else if (h < 120.0f) {
        r = x; g = c;
    } else if (h < 180.0f) {
        g = c; b = x;
    } else if (h < 240.0f) {
        g = x; b = c;
    } else if (h < 300.0f) {
        r = x; b = c;
    } else {
        r = c; b = x;
    }

    return Gdiplus::Color(alpha,
        (BYTE)((r + m) * 255.0f),
        (BYTE)((g + m) * 255.0f),
        (BYTE)((b + m) * 255.0f));
}

Gdiplus::Color ParticleTrailOverlayLayer::ArgbWithOpacity(uint32_t argb, float opacityScale) {
    const int baseAlpha = static_cast<int>((argb >> 24) & 0xFFu);
    const int alpha = std::clamp(
        static_cast<int>(std::lround(static_cast<float>(baseAlpha) * opacityScale)),
        0,
        255);
    return Gdiplus::Color(
        static_cast<BYTE>(alpha),
        static_cast<BYTE>((argb >> 16) & 0xFFu),
        static_cast<BYTE>((argb >> 8) & 0xFFu),
        static_cast<BYTE>(argb & 0xFFu));
}

void ParticleTrailOverlayLayer::Emit(const TrailEffectRenderCommand& command, int count) {
    globalHue_ = std::fmod(globalHue_ + 5.0f, 360.0f);
    const float intensityScale = std::clamp(static_cast<float>(0.75 + command.intensity * 0.60), 0.60f, 1.80f);
    const float sizeScale = std::clamp(static_cast<float>(command.sizePx / 56.0), 0.45f, 2.40f);
    const float durationSec = std::clamp(static_cast<float>(command.durationSec), 0.08f, 3.0f);
    const float decayScale = std::clamp(0.22f / durationSec, 0.35f, 2.5f);
    const uint32_t baseArgb = command.strokeArgb != 0 ? command.strokeArgb : command.fillArgb;
    const bool useHue = isChromatic_;
    const float spawnOpacity = std::clamp(static_cast<float>(command.baseOpacity), 0.10f, 1.0f);

    for (int i = 0; i < count; ++i) {
        Particle particle{};
        // Keep particle positions in screen space.
        // OverlayHost may render the same layer onto multiple per-monitor windows.
        // Converting to local coordinates here can bind points to the wrong monitor origin.
        particle.x = static_cast<float>(command.overlayPoint.x);
        particle.y = static_cast<float>(command.overlayPoint.y);

        const auto spawn = trail_style_compute::ComputeParticleSpawnMetrics(
            &rngState_,
            isChromatic_,
            globalHue_);
        const float speed = static_cast<float>(spawn.speedPxPerTick) * intensityScale;
        particle.vx = std::cos(static_cast<float>(spawn.angleRad)) * speed;
        particle.vy = std::sin(static_cast<float>(spawn.angleRad)) * speed;
        particle.life = 1.0f;
        particle.size = static_cast<float>(spawn.sizePx) * sizeScale;
        particle.hue = static_cast<float>(spawn.hueDeg);
        particle.renderRadiusPx = particle.size * 0.5f;
        particle.renderOpacity = spawnOpacity;
        particle.decayScale = decayScale;
        particle.baseArgb = baseArgb;
        particle.useHue = useHue;

        particles_.push_back(particle);
    }
}

} // namespace mousefx
