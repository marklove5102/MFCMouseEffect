#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include <cmath>

namespace mousefx {

class RippleRenderer : public IRippleRenderer {
public:
    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;

        const float eased = 1.0f - (1.0f - Clamp01(t)) * (1.0f - Clamp01(t)) * (1.0f - Clamp01(t)); // EaseOutCubic manual
        const float alpha = 1.0f - eased;
        const float radius = style.startRadius + (style.endRadius - style.startRadius) * eased;
        const float cx = sizePx / 2.0f;
        const float cy = sizePx / 2.0f;

        // Fill
        Gdiplus::Color base = ToGdiPlus(style.fill);
        const BYTE aCenter = ClampByte((int)(base.GetA() * alpha));
        const Gdiplus::Color center(aCenter, base.GetR(), base.GetG(), base.GetB());
        const Gdiplus::Color edge(0, base.GetR(), base.GetG(), base.GetB());

        Gdiplus::GraphicsPath path;
        path.AddEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        Gdiplus::PathGradientBrush pgb(&path);
        pgb.SetCenterColor(center);
        Gdiplus::Color surround[1] = { edge };
        int count = 1;
        pgb.SetSurroundColors(surround, &count);
        pgb.SetCenterPoint(Gdiplus::PointF(cx, cy));
        g.FillPath(&pgb, &path);

        // Stroke
        Gdiplus::Color stroke = ToGdiPlus(style.stroke);
        stroke = Gdiplus::Color(ClampByte((int)(stroke.GetA() * alpha)), stroke.GetR(), stroke.GetG(), stroke.GetB());
        
        // Glow (soft shadow)
        const Gdiplus::Color glow = ToGdiPlus(style.glow);
        for (int i = 0; i < 3; i++) {
            const float w = style.strokeWidth + 10.0f + i * 4.0f;
            BYTE a = ClampByte((int)(glow.GetA() * alpha * (0.35f - i * 0.08f)));
            Gdiplus::Pen p(Gdiplus::Color(a, glow.GetR(), glow.GetG(), glow.GetB()), w);
            p.SetLineJoin(Gdiplus::LineJoinRound);
            g.DrawEllipse(&p, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        }

        Gdiplus::Pen pen(stroke, style.strokeWidth);
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawEllipse(&pen, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }
};

REGISTER_RENDERER("ripple", RippleRenderer)

} // namespace mousefx
