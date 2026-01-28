#pragma once

#include "IRippleRenderer.h"
#include "RenderStrategies.h" // For utils

namespace mousefx {

// 4. Standard Ripple Renderer
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

// 5. Star Renderer (Legacy IconStar)
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

// 6. Scroll Chevron Renderer
class ChevronRenderer : public IRippleRenderer {
public:
    RenderParams params_;
    
    // Allow external setting of params if needed, or pass via RenderContext if we had one.
    // Since StartContinuous taking RenderParams is used, we can store it.
    // BUT IRippleRenderer interface doesn't have SetParams. We can add it or just depend on RippleStyle?
    // The original code passed 'RenderParams' to StartContinuous.
    // Let's add UpdateParams to interface or just assume we set it after creation.
    
    void SetParams(const RenderParams& p) { params_ = p; }

    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;
        
        const float eased = 1.0f - (1.0f - Clamp01(t)) * (1.0f - Clamp01(t)) * (1.0f - Clamp01(t));
        const float intensity = Clamp01(params_.intensity);
        const float alpha = (1.0f - eased) * intensity;
        const float radius = style.startRadius + (style.endRadius - style.startRadius) * eased;
        const float cx = sizePx / 2.0f;
        const float cy = sizePx / 2.0f;

        const float dir = params_.directionRad;
        const float dx = (float)cos(dir);
        const float dy = (float)sin(dir);
        const float px = -dy;
        const float py = dx;

        const Gdiplus::Color base = ToGdiPlus(style.stroke);
        const BYTE aBase = ClampByte((int)(base.GetA() * alpha));

        for (int i = 0; i < 3; ++i) {
            const float offset = i * (radius * 0.25f);
            const float length = radius * 1.1f;
            const float halfWidth = style.strokeWidth * (3.2f - i * 0.6f);

            const float tipX = cx + dx * (length * 0.5f - offset);
            const float tipY = cy + dy * (length * 0.5f - offset);
            const float tailX = cx - dx * (length * 0.5f + offset);
            const float tailY = cy - dy * (length * 0.5f + offset);

            const float lx = tailX + px * halfWidth;
            const float ly = tailY + py * halfWidth;
            const float rx = tailX - px * halfWidth;
            const float ry = tailY - py * halfWidth;

            const float fade = 1.0f - i * 0.18f;
            const Gdiplus::Color glow = ToGdiPlus(style.glow);
            const BYTE a = ClampByte((int)(aBase * fade));
            const BYTE glowA = ClampByte((int)(glow.GetA() * alpha * fade));
            Gdiplus::Pen glowPen(Gdiplus::Color(glowA, glow.GetR(), glow.GetG(), glow.GetB()),
                style.strokeWidth + 6.0f);
            glowPen.SetLineJoin(Gdiplus::LineJoinRound);
            g.DrawLine(&glowPen, tipX, tipY, lx, ly);
            g.DrawLine(&glowPen, tipX, tipY, rx, ry);

            Gdiplus::Pen pen(Gdiplus::Color(a, base.GetR(), base.GetG(), base.GetB()),
                style.strokeWidth + 1.0f);
            pen.SetLineJoin(Gdiplus::LineJoinRound);
            g.DrawLine(&pen, tipX, tipY, lx, ly);
            g.DrawLine(&pen, tipX, tipY, rx, ry);
        }
    }
};

// 7. Hover Crosshair Renderer
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

} // namespace mousefx
