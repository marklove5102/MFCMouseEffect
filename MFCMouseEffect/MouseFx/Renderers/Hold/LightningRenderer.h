#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace mousefx {

class LightningRenderer : public IRippleRenderer {
public:
    struct Particle {
        float angle;
        float speed;
        float phase;
        float distOffset;
        bool active;
    };
    std::vector<Particle> particles_;

    void Start(const RippleStyle& style) override {
        // Initialize particles
        particles_.clear();
        for (int i = 0; i < 24; ++i) {
            Particle p;
            p.angle = ((float)(rand() % 360) / 180.0f) * 3.14159f;
            p.speed = 1.0f + ((float)(rand() % 100) / 50.0f); // 1.0 - 3.0
            p.phase = ((float)(rand() % 100) / 100.0f);
            p.distOffset = ((float)(rand() % 100) / 100.0f) * 5.0f;
            p.active = true;
            particles_.push_back(p);
        }
    }

    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;
        
        const float cx = sizePx / 2.0f;
        const float cy = sizePx / 2.0f;
        const float radius = style.endRadius;
        
        // Central Core
        float coreRadius = style.strokeWidth * (2.0f + t * 4.0f); // 2x -> 6x
        float pulse = 0.5f + 0.5f * (float)sin((double)elapsedMs * 0.05); // Fast pulse
        float coreAlpha = 0.8f + 0.2f * pulse;
        
        Gdiplus::Color glow = ToGdiPlus(style.glow);
        Gdiplus::Color coreColor = ToGdiPlus(style.stroke);
        
        // Core Glow
        {
             Gdiplus::GraphicsPath path;
             path.AddEllipse(cx - coreRadius * 2.5f, cy - coreRadius * 2.5f, coreRadius * 5.0f, coreRadius * 5.0f);
             Gdiplus::PathGradientBrush pgb(&path);
             pgb.SetCenterColor(Gdiplus::Color(ClampByte((int)(glow.GetA() * 0.6f)), glow.GetR(), glow.GetG(), glow.GetB()));
             Gdiplus::Color surround[1] = { Gdiplus::Color(0, glow.GetR(), glow.GetG(), glow.GetB()) };
             int count = 1;
             pgb.SetSurroundColors(surround, &count);
             pgb.SetCenterPoint(Gdiplus::PointF(cx, cy));
             g.FillPath(&pgb, &path);
        }

        // Draw Core (Optimized: Energy Orb instead of solid blob)
        {
             Gdiplus::GraphicsPath path;
             float drawRadius = coreRadius * 1.5f; 
             path.AddEllipse(cx - drawRadius, cy - drawRadius, drawRadius * 2.0f, drawRadius * 2.0f);
             
             Gdiplus::PathGradientBrush pgb(&path);
             
             BYTE r = (BYTE)std::min(255, (int)coreColor.GetR() + 100);
             BYTE gVal = (BYTE)std::min(255, (int)coreColor.GetG() + 100);
             BYTE bVal = (BYTE)std::min(255, (int)coreColor.GetB() + 100);
             
             pgb.SetCenterColor(Gdiplus::Color(ClampByte((int)(coreAlpha * 255)), r, gVal, bVal));
             
             Gdiplus::Color surround[1] = { Gdiplus::Color(0, coreColor.GetR(), coreColor.GetG(), coreColor.GetB()) };
             int count = 1;
             pgb.SetSurroundColors(surround, &count);
             pgb.SetCenterPoint(Gdiplus::PointF(cx, cy));
             pgb.SetFocusScales(0.4f, 0.4f); 
             
             g.FillPath(&pgb, &path);
        }
        
        srand((unsigned int)elapsedMs);
        
        Gdiplus::Pen boltPen(Gdiplus::Color(ClampByte((int)(coreColor.GetA() * 0.7f)), coreColor.GetR(), coreColor.GetG(), coreColor.GetB()), style.strokeWidth * 0.5f);
        
        for (auto& p : particles_) {
            float linearProgress = (fnmod((float)elapsedMs * 0.001f * p.speed + p.phase, 1.0f)); 
            float distFactor = 1.0f - linearProgress;
            
            float currentDist = radius * 0.3f + (radius * 0.8f) * distFactor + p.distOffset * 10.0f;
            if (currentDist > radius) currentDist = radius; 
            
            float angle = p.angle;
            
            float length = 15.0f * (1.0f + p.speed);
            
            float x1 = cx + (float)cos(angle) * currentDist;
            float y1 = cy + (float)sin(angle) * currentDist;
            float x2 = cx + (float)cos(angle) * (currentDist + length);
            float y2 = cy + (float)sin(angle) * (currentDist + length);
            
            float jitter = 2.0f * ((float)(rand() % 100) / 100.0f - 0.5f);
            
            g.DrawLine(&boltPen, x1 + jitter, y1 + jitter, x2, y2);
        }
    }
};

REGISTER_RENDERER("lightning", LightningRenderer)

} // namespace mousefx
