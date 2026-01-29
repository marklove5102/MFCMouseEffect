#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include <cmath>

namespace mousefx {

class HexRenderer : public IRippleRenderer {
public:
    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;

        const float cx = sizePx / 2.0f;
        const float cy = sizePx / 2.0f;
        const float radius = style.endRadius; // Base radius
        const float progress = Clamp01(t); 
        
        const float pulse = 0.5f + 0.5f * (float)sin((double)elapsedMs * 0.003);
        const float alphaBase = 0.6f + 0.4f * pulse;
        
        Gdiplus::Color stroke = ToGdiPlus(style.stroke);
        Gdiplus::Color glow = ToGdiPlus(style.glow);
        Gdiplus::Color fill = ToGdiPlus(style.fill); 
        
        auto DrawHex = [&](float r, float rotRad, float width, float aFactor, bool fillHex) {
            Gdiplus::PointF pts[6];
            for (int i = 0; i < 6; ++i) {
                float angle = rotRad + (float)i * (3.14159f / 3.0f);
                pts[i].X = cx + r * (float)cos(angle);
                pts[i].Y = cy + r * (float)sin(angle);
            }
            
            BYTE a = ClampByte((int)(stroke.GetA() * alphaBase * aFactor));
            Gdiplus::Color c(a, stroke.GetR(), stroke.GetG(), stroke.GetB());
            
            if (fillHex) {
                 BYTE fa = ClampByte((int)(fill.GetA() * alphaBase * aFactor * 0.3f));
                 Gdiplus::SolidBrush b(Gdiplus::Color(fa, fill.GetR(), fill.GetG(), fill.GetB()));
                 g.FillPolygon(&b, pts, 6);
            }

            Gdiplus::Pen p(c, width);
            p.SetLineJoin(Gdiplus::LineJoinMiter);
            g.DrawPolygon(&p, pts, 6);
            
            BYTE ga = ClampByte((int)(glow.GetA() * alphaBase * aFactor));
            Gdiplus::SolidBrush gb(Gdiplus::Color(ga, glow.GetR(), glow.GetG(), glow.GetB()));
            float dotR = width * 1.5f;
            for(int i=0; i<6; ++i) {
                g.FillEllipse(&gb, pts[i].X - dotR, pts[i].Y - dotR, dotR*2, dotR*2);
            }
        };
        
        float timeSec = (float)elapsedMs / 1000.0f;
        
        float r1 = radius * 0.4f * progress;
        if (r1 > 1.0f) DrawHex(r1, timeSec * 2.0f, style.strokeWidth, 1.0f, true);
        
        float r2 = radius * 0.7f * progress;
        if (r2 > 1.0f) DrawHex(r2, -timeSec * 0.5f, style.strokeWidth * 0.8f, 0.7f, false);
        
        float r3 = radius * 1.0f * progress;
        if (r3 > 1.0f) DrawHex(r3, timeSec * 1.0f + 0.5f, style.strokeWidth * 0.5f, 0.4f, false);
    }
};

REGISTER_RENDERER("hex", HexRenderer)

} // namespace mousefx
