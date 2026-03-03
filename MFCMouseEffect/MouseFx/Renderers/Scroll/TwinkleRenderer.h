#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include "MouseFx/Core/Effects/ScrollTwinkleFrameCompute.h"

namespace mousefx {

class TwinkleRenderer : public IRippleRenderer {
public:
    void SetParams(const RenderParams& params) override { params_ = params; }

    void Start(const RippleStyle& style) override {
        (void)style;
        started_ = true;
    }

    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;

        if (!started_) {
            Start(style);
        }

        const float intensity = Clamp01(params_.intensity);
        const float cx = sizePx * 0.5f;
        const float cy = sizePx * 0.5f;

        if (!stateStarted_) {
            state_.Start(cx, cy, params_.directionRad, intensity);
            stateStarted_ = true;
        }

        const Gdiplus::Color stroke = ToGdiPlus(style.stroke);
        const Gdiplus::Color glow = ToGdiPlus(style.glow);

        // Compute platform-agnostic frame data from shared Core
        auto frame = state_.ComputeFrame(
            t, elapsedMs, sizePx,
            stroke.GetR() / 255.0f, stroke.GetG() / 255.0f, stroke.GetB() / 255.0f,
            glow.GetR() / 255.0f, glow.GetG() / 255.0f, glow.GetB() / 255.0f);

        // Draw particles
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        for (const auto& p : frame.particles) {
            const BYTE coreA = ClampByte(static_cast<int>(p.coreAlpha * 255.0f));
            if (coreA <= 2) continue;

            // Trail line
            {
                const BYTE tA = ClampByte(static_cast<int>(p.trailAlpha * 255.0f));
                Gdiplus::Pen trailPen(Gdiplus::Color(tA,
                    ClampByte(static_cast<int>(p.trailR * 255.0f)),
                    ClampByte(static_cast<int>(p.trailG * 255.0f)),
                    ClampByte(static_cast<int>(p.trailB * 255.0f))),
                    std::max(0.75f, p.drawSize * 0.26f));
                trailPen.SetStartCap(Gdiplus::LineCapRound);
                trailPen.SetEndCap(Gdiplus::LineCapRound);
                g.DrawLine(&trailPen, p.prevX, p.prevY, p.x, p.y);
            }

            // Glow circle
            {
                const BYTE gA = ClampByte(static_cast<int>(p.glowAlpha * 255.0f));
                const float gs = p.drawSize * 1.22f;
                Gdiplus::SolidBrush glowBrush(Gdiplus::Color(gA,
                    ClampByte(static_cast<int>(p.trailR * 255.0f)),
                    ClampByte(static_cast<int>(p.trailG * 255.0f)),
                    ClampByte(static_cast<int>(p.trailB * 255.0f))));
                g.FillEllipse(&glowBrush,
                    p.x - gs * 0.5f, p.y - gs * 0.5f, gs, gs);
            }

            // Core dot
            {
                Gdiplus::SolidBrush coreBrush(Gdiplus::Color(coreA,
                    ClampByte(static_cast<int>(p.coreR * 255.0f)),
                    ClampByte(static_cast<int>(p.coreG * 255.0f)),
                    ClampByte(static_cast<int>(p.coreB * 255.0f))));
                g.FillEllipse(&coreBrush,
                    p.x - p.drawSize * 0.5f, p.y - p.drawSize * 0.5f,
                    p.drawSize, p.drawSize);
            }
        }
    }

private:
    RenderParams params_{};
    ScrollTwinkleState state_;
    bool started_ = false;
    bool stateStarted_ = false;
};

REGISTER_RENDERER("twinkle", TwinkleRenderer)

} // namespace mousefx
