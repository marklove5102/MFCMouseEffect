#pragma once
/// ScrollTwinkleFrameCompute.h
/// Platform-agnostic per-frame twinkle particle computation.
/// Maintains particle state (emission, physics, lifecycle).
/// Produces a list of draw-ready particles each frame.

#include <cmath>
#include <cstdint>
#include <vector>

namespace mousefx {

// ── Twinkle frame data (platform-independent) ─────────────────

struct ScrollTwinkleDrawParticle {
    float x, y, prevX, prevY, drawSize;
    float coreR, coreG, coreB, coreAlpha;
    float trailR, trailG, trailB, trailAlpha;
    float glowAlpha;
};

struct ScrollTwinkleFrameData {
    std::vector<ScrollTwinkleDrawParticle> particles;
};

// ── Twinkle math helpers ──────────────────────────────────────

namespace twinkle_math {

inline float Clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

inline float Smoothstep(float a, float b, float x) {
    if (b <= a) return x >= b ? 1.0f : 0.0f;
    float u = (x - a) / (b - a);
    if (u < 0.0f) u = 0.0f;
    if (u > 1.0f) u = 1.0f;
    return u * u * (3.0f - 2.0f * u);
}

inline float Mix(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

} // namespace twinkle_math

// ── Stateful particle system ──────────────────────────────────

class ScrollTwinkleState {
public:
    void Start(float cx, float cy, float dirRad, float intensity) {
        particles_.clear();
        emitted_ = false;
        lastElapsedMs_ = 0;
        cx_ = cx;
        cy_ = cy;
        dirX_ = std::cos(dirRad);
        dirY_ = std::sin(dirRad);
        prpX_ = -dirY_;
        prpY_ = dirX_;
        intensity_ = twinkle_math::Clamp01(intensity);
        rngState_ ^= NextSeed();
        if (rngState_ == 0u) rngState_ = 0xA341316Cu;
    }

    ScrollTwinkleFrameData ComputeFrame(
        float t,
        uint64_t elapsedMs,
        int sizePx,
        float strokeR, float strokeG, float strokeB,
        float glowR, float glowG, float glowB
    ) {
        using namespace twinkle_math;
        ScrollTwinkleFrameData frame;

        const float tn = Clamp01(t);
        const float lifeFade = 1.0f - Smoothstep(0.70f, 1.0f, tn);
        if (lifeFade <= 0.001f) {
            particles_.clear();
            return frame;
        }

        if (!emitted_) {
            EmitBurst();
            emitted_ = true;
        }
        UpdateParticles(elapsedMs, sizePx);

        const float baseSize = 0.78f + 1.20f * intensity_;
        const float seconds = static_cast<float>(elapsedMs) / 1000.0f;

        frame.particles.reserve(particles_.size());
        for (const auto& p : particles_) {
            const float life = 1.0f - Clamp01(p.ageMs / p.lifeMs);
            if (life <= 0.0f) continue;

            const float flicker = 0.86f + 0.14f * std::sin(seconds * 9.0f + p.flicker);
            const float drawSize = baseSize * p.size * (0.55f + 0.45f * life) * flicker;
            if (drawSize <= 0.08f) continue;

            const float coreAlpha = (178.0f / 255.0f) * life * lifeFade;
            if (coreAlpha <= 0.008f) continue;

            const float mixT = 0.05f + (1.0f - life) * 0.14f;
            const float cR = Mix(strokeR, 1.0f, mixT);
            const float cG = Mix(strokeG, 1.0f, mixT);
            const float cB = Mix(strokeB, 1.0f, mixT);
            const float trailMixT = 0.38f;
            const float tR = Mix(glowR, cR, trailMixT);
            const float tG = Mix(glowG, cG, trailMixT);
            const float tB = Mix(glowB, cB, trailMixT);

            frame.particles.push_back({
                p.x, p.y, p.prevX, p.prevY, drawSize,
                cR, cG, cB, coreAlpha,
                tR, tG, tB, coreAlpha * 0.64f,
                coreAlpha * 0.14f
            });
        }
        return frame;
    }

private:
    struct Particle {
        float x = 0.0f, y = 0.0f;
        float prevX = 0.0f, prevY = 0.0f;
        float vx = 0.0f, vy = 0.0f;
        float size = 0.0f;
        float ageMs = 0.0f, lifeMs = 1.0f;
        float flicker = 0.0f;
    };

    static constexpr float kConeHalfRad = 3.1415926535f / 6.0f;

    static uint32_t NextSeed() {
        static uint32_t seed = 0xC13FA9A9u;
        seed = seed * 1664525u + 1013904223u;
        return seed;
    }

    float Rand01() {
        rngState_ ^= (rngState_ << 13);
        rngState_ ^= (rngState_ >> 17);
        rngState_ ^= (rngState_ << 5);
        return static_cast<float>(rngState_ & 0x00FFFFFFu) / 16777216.0f;
    }

    void EmitBurst() {
        const int count = static_cast<int>(10.0f + intensity_ * 12.0f);
        particles_.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i) {
            const float co = (Rand01() * 2.0f - 1.0f) * kConeHalfRad;
            const float c = std::cos(co), s = std::sin(co);
            const float sprayX = dirX_ * c + prpX_ * s;
            const float sprayY = dirY_ * c + prpY_ * s;
            const float sj = (Rand01() * 2.0f - 1.0f) * (1.4f + 3.0f * intensity_);
            const float bj = Rand01() * (2.5f + 5.0f * intensity_);
            const float speed = 4.2f + Rand01() * (4.8f + 2.2f * intensity_);
            Particle p{};
            p.x = cx_ + prpX_ * sj - dirX_ * bj;
            p.y = cy_ + prpY_ * sj - dirY_ * bj;
            p.prevX = p.x; p.prevY = p.y;
            p.vx = sprayX * speed; p.vy = sprayY * speed;
            p.size = 1.0f + Rand01() * 1.4f;
            p.lifeMs = 95.0f + Rand01() * 120.0f;
            p.flicker = Rand01() * 6.2831853f;
            particles_.push_back(p);
        }
    }

    void UpdateParticles(uint64_t elapsedMs, int sizePx) {
        uint64_t dtMs = (lastElapsedMs_ == 0 || elapsedMs <= lastElapsedMs_) ? 16u : (elapsedMs - lastElapsedMs_);
        if (dtMs > 34u) dtMs = 34u;
        lastElapsedMs_ = elapsedMs;

        const float step = static_cast<float>(dtMs) / 16.0f;
        const float drag = std::pow(0.90f, step);
        const float fL = -sizePx * 0.5f, fR = sizePx * 1.5f;
        const float fT = -sizePx * 0.5f, fB = sizePx * 1.5f;
        const float flow = 0.015f * step;
        const float turb = 0.040f * step;

        for (size_t i = 0; i < particles_.size();) {
            auto& p = particles_[i];
            p.ageMs += static_cast<float>(dtMs);
            if (p.ageMs >= p.lifeMs) {
                particles_[i] = particles_.back();
                particles_.pop_back();
                continue;
            }
            p.prevX = p.x; p.prevY = p.y;
            p.vx = p.vx * drag + dirX_ * flow + prpX_ * ((Rand01() * 2.0f - 1.0f) * turb);
            p.vy = p.vy * drag + dirY_ * flow + (Rand01() * 2.0f - 1.0f) * turb;
            p.x += p.vx * step; p.y += p.vy * step;
            p.flicker += 0.16f * step;
            if (p.x < fL || p.x > fR || p.y < fT || p.y > fB) {
                particles_[i] = particles_.back();
                particles_.pop_back();
                continue;
            }
            ++i;
        }
    }

    float cx_ = 0, cy_ = 0;
    float dirX_ = 0, dirY_ = 0;
    float prpX_ = 0, prpY_ = 0;
    float intensity_ = 0;
    std::vector<Particle> particles_;
    bool emitted_ = false;
    uint64_t lastElapsedMs_ = 0;
    uint32_t rngState_ = 0x9A53C11Du;
};

} // namespace mousefx
