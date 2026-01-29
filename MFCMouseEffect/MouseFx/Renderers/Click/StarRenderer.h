#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include <cmath>

namespace mousefx {

class StarRenderer : public IRippleRenderer {
public:
    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;
        
        const float eased = 1.0f - (1.0f - Clamp01(t)) * (1.0f - Clamp01(t)) * (1.0f - Clamp01(t));
        const float alpha = 1.0f - eased;
        const float radius = style.startRadius + (style.endRadius - style.startRadius) * eased;
        const float cx = sizePx / 2.0f;
        const float cy = sizePx / 2.0f;

        Gdiplus::Color fill = ToGdiPlus(style.fill);
        BYTE alphaByte = ClampByte((int)(255 * alpha));
        fill = Gdiplus::Color(alphaByte, fill.GetR(), fill.GetG(), fill.GetB());

        Gdiplus::PointF pts[10];
        const float rOuter = radius;
        const float rInner = radius * 0.4f;
        const double PI = 3.14159265358979323846;
        for (int i = 0; i < 10; i++) {
            double angle = i * PI / 5.0 - PI / 2.0; 
            float r = (i % 2 == 0) ? rOuter : rInner;
            pts[i] = Gdiplus::PointF(cx + r * (float)cos(angle), cy + r * (float)sin(angle));
        }

        Gdiplus::SolidBrush brush(fill);
        g.FillPolygon(&brush, pts, 10);

        Gdiplus::Color stroke = ToGdiPlus(style.stroke);
        stroke = Gdiplus::Color(alphaByte, stroke.GetR(), stroke.GetG(), stroke.GetB());
        Gdiplus::Pen pen(stroke, style.strokeWidth);
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawPolygon(&pen, pts, 10);
    }
};

REGISTER_RENDERER("star", StarRenderer)

} // namespace mousefx
