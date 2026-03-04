#pragma once

#include "MouseFx/Core/Effects/TrailEffectCompute.h"
#include "MouseFx/Interfaces/IOverlayLayer.h"

#include <vector>

namespace mousefx {

class ParticleTrailOverlayLayer final : public IOverlayLayer {
public:
    explicit ParticleTrailOverlayLayer(bool isChromatic);

    void AddCommand(const TrailEffectRenderCommand& command);
    void Clear();

    void Update(uint64_t nowMs) override;
    void Render(Gdiplus::Graphics& graphics) override;
    bool IsAlive() const override { return true; }

private:
    struct Particle {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float life = 1.0f;
        float hue = 0.0f;
        float size = 1.0f;
        float renderRadiusPx = 0.5f;
        float renderOpacity = 1.0f;
        float decayScale = 1.0f;
        uint32_t baseArgb = 0;
        bool useHue = false;
    };

    static Gdiplus::Color HslToRgb(float h, float s, float l, BYTE alpha);
    static Gdiplus::Color ArgbWithOpacity(uint32_t argb, float opacityScale);
    void Emit(const TrailEffectRenderCommand& command, int count);

    bool isChromatic_ = false;
    std::vector<Particle> particles_{};
    uint64_t lastTickMs_ = 0;
    float globalHue_ = 0.0f;
    uint32_t rngState_ = 0x7F4A7C15u;
};

} // namespace mousefx
