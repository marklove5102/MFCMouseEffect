#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include <cmath>

namespace mousefx {

class CrosshairRenderer : public IRippleRenderer {
public:
    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;
        
        const float cx = sizePx / 2.0f;
        const float cy = sizePx / 2.0f;
        const float radius = style.endRadius * 0.9f;
        const float phase = (float)fmod((double)elapsedMs / (double)style.durationMs, 1.0);
        const float pulse = 0.4f + 0.6f * (float)sin(phase * 2.0f * 3.1415926f);
        const float alpha = Clamp01(pulse);

        const Gdiplus::Color stroke = ToGdiPlus(style.stroke);
        const BYTE aStroke = ClampByte((int)(stroke.GetA() * alpha));
        const Gdiplus::Color glow = ToGdiPlus(style.glow);
        const BYTE aGlow = ClampByte((int)(glow.GetA() * alpha * 0.6f));

        Gdiplus::Pen glowPen(Gdiplus::Color(aGlow, glow.GetR(), glow.GetG(), glow.GetB()),
            style.strokeWidth + 6.0f);
        glowPen.SetLineJoin(Gdiplus::LineJoinRound);

        Gdiplus::Pen pen(Gdiplus::Color(aStroke, stroke.GetR(), stroke.GetG(), stroke.GetB()),
            style.strokeWidth + 0.5f);
        pen.SetLineJoin(Gdiplus::LineJoinRound);

        const float tick = radius * 0.22f;
        g.DrawLine(&glowPen, cx - radius, cy, cx - radius + tick, cy);
        g.DrawLine(&glowPen, cx + radius - tick, cy, cx + radius, cy);
        g.DrawLine(&glowPen, cx, cy - radius, cx, cy - radius + tick);
        g.DrawLine(&glowPen, cx, cy + radius - tick, cx, cy + radius);

        g.DrawLine(&pen, cx - radius, cy, cx - radius + tick, cy);
        g.DrawLine(&pen, cx + radius - tick, cy, cx + radius, cy);
        g.DrawLine(&pen, cx, cy - radius, cx, cy - radius + tick);
        g.DrawLine(&pen, cx, cy + radius - tick, cx, cy + radius);

        // Orbiting dot
        const float dotAngle = phase * 2.0f * 3.1415926f;
        const float dotR = style.strokeWidth * 1.6f;
        const float dotX = cx + (float)cos(dotAngle) * (radius * 0.6f);
        const float dotY = cy + (float)sin(dotAngle) * (radius * 0.6f);
        Gdiplus::SolidBrush dotBrush(Gdiplus::Color(aStroke, stroke.GetR(), stroke.GetG(), stroke.GetB()));
        g.FillEllipse(&dotBrush, dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
    }
};

REGISTER_RENDERER("glow", CrosshairRenderer)

} // namespace mousefx
