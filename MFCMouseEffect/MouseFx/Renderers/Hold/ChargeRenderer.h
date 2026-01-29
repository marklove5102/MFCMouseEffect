#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include <cmath>

namespace mousefx {

class ChargeRenderer : public IRippleRenderer {
public:
    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;
        
        const float cx = sizePx / 2.0f;
        const float cy = sizePx / 2.0f;
        const float radius = style.endRadius;
        const float progress = Clamp01(t);
        const float pulse = 0.5f + 0.5f * (float)sin((double)elapsedMs * 0.0045);
        const float alpha = 0.55f + 0.45f * pulse;

        const Gdiplus::Color stroke = ToGdiPlus(style.stroke);
        const BYTE aStroke = ClampByte((int)(stroke.GetA() * alpha));
        const Gdiplus::Color glow = ToGdiPlus(style.glow);
        const BYTE aGlow = ClampByte((int)(glow.GetA() * alpha * 0.6f));

        // Background ring
        Gdiplus::Pen bgPen(Gdiplus::Color(ClampByte((int)(aStroke * 0.25f)),
            stroke.GetR(), stroke.GetG(), stroke.GetB()), style.strokeWidth);
        bgPen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawEllipse(&bgPen, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        // Glow arc
        Gdiplus::Pen glowPen(Gdiplus::Color(aGlow, glow.GetR(), glow.GetG(), glow.GetB()),
            style.strokeWidth + 8.0f);
        glowPen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawArc(&glowPen, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, -90.0f, progress * 360.0f);

        // Progress arc
        Gdiplus::Pen pen(Gdiplus::Color(aStroke, stroke.GetR(), stroke.GetG(), stroke.GetB()),
            style.strokeWidth + 1.0f);
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawArc(&pen, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, -90.0f, progress * 360.0f);

        // Accent dot at head
        const float angle = (-90.0f + progress * 360.0f) * 3.1415926f / 180.0f;
        const float dotX = cx + (float)cos(angle) * radius;
        const float dotY = cy + (float)sin(angle) * radius;
        const float dotR = style.strokeWidth * 2.2f;
        Gdiplus::SolidBrush dotBrush(Gdiplus::Color(aStroke, stroke.GetR(), stroke.GetG(), stroke.GetB()));
        g.FillEllipse(&dotBrush, dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
    }
};

REGISTER_RENDERER("charge", ChargeRenderer)

} // namespace mousefx
