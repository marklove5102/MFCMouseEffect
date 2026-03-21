#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include <cmath>

namespace mousefx {

class RippleRenderer : public IRippleRenderer {
public:
    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;

        const float clampedT = Clamp01(t);
        const float eased = 1.0f - (1.0f - clampedT) * (1.0f - clampedT) * (1.0f - clampedT);
        const float fadeAlpha = std::max(0.0f, 1.0f - clampedT);
        const float radius = style.startRadius + (style.endRadius - style.startRadius) * eased;
        const float cx = sizePx / 2.0f;
        const float cy = sizePx / 2.0f;
        const float ringWidth = std::max(
            style.strokeWidth * 1.34f,
            std::min(radius * 0.18f, std::max(style.strokeWidth + 1.25f, 2.6f)));
        Gdiplus::GraphicsPath ringPath;
        ringPath.AddEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        Gdiplus::Color stroke = ToGdiPlus(style.stroke);
        stroke = Gdiplus::Color(
            ClampByte(static_cast<int>(stroke.GetA() * fadeAlpha)),
            stroke.GetR(),
            stroke.GetG(),
            stroke.GetB());

        // Outer glow halo.
        const Gdiplus::Color glow = ToGdiPlus(style.glow);
        for (int i = 0; i < 3; ++i) {
            const float w = ringWidth + 5.0f + i * 2.8f;
            const BYTE a = ClampByte(static_cast<int>(glow.GetA() * fadeAlpha * (0.15f - i * 0.03f)));
            Gdiplus::Pen p(Gdiplus::Color(a, glow.GetR(), glow.GetG(), glow.GetB()), w);
            p.SetLineJoin(Gdiplus::LineJoinRound);
            p.SetStartCap(Gdiplus::LineCapRound);
            p.SetEndCap(Gdiplus::LineCapRound);
            g.DrawEllipse(&p, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        }

        Gdiplus::Pen pen(stroke, std::max(style.strokeWidth * 0.94f, ringWidth * 0.18f));
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        pen.SetStartCap(Gdiplus::LineCapRound);
        pen.SetEndCap(Gdiplus::LineCapRound);
        g.DrawEllipse(&pen, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }
};

REGISTER_RENDERER("ripple", RippleRenderer)

} // namespace mousefx
