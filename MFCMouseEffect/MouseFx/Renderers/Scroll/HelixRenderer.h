#pragma once

#include "../RenderUtils.h"
#include "../RendererRegistry.h"
#include "MouseFx/Core/Effects/ScrollHelixFrameCompute.h"

namespace mousefx {

class HelixRenderer : public IRippleRenderer {
public:
    void SetParams(const RenderParams& params) override { params_ = params; }

    void Render(Gdiplus::Graphics& g, float t, uint64_t elapsedMs, int sizePx, const RippleStyle& style) override {
        using namespace render_utils;

        const float intensity = Clamp01(params_.intensity);
        if (intensity <= 0.01f) return;

        const Gdiplus::Color stroke = ToGdiPlus(style.stroke);
        const Gdiplus::Color glow = ToGdiPlus(style.glow);

        // Compute platform-agnostic frame data from shared Core
        auto frame = ComputeHelixFrame(
            t, elapsedMs, sizePx,
            params_.directionRad, intensity,
            style.startRadius, style.endRadius, style.strokeWidth,
            stroke.GetR() / 255.0f, stroke.GetG() / 255.0f, stroke.GetB() / 255.0f,
            glow.GetR() / 255.0f, glow.GetG() / 255.0f, glow.GetB() / 255.0f);

        // Draw segments (already z-sorted)
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        for (const auto& s : frame.segments) {
            const BYTE a = ClampByte(static_cast<int>(s.a * 255.0f));
            if (a == 0) continue;
            const BYTE r = ClampByte(static_cast<int>(s.r * 255.0f));
            const BYTE gb = ClampByte(static_cast<int>(s.g * 255.0f));
            const BYTE b = ClampByte(static_cast<int>(s.b * 255.0f));

            // Soft aura
            {
                const BYTE auraA = ClampByte(static_cast<int>(a * 0.24f));
                Gdiplus::Pen auraPen(Gdiplus::Color(auraA, r, gb, b),
                    std::max(1.0f, s.width + 2.1f));
                auraPen.SetStartCap(Gdiplus::LineCapRound);
                auraPen.SetEndCap(Gdiplus::LineCapRound);
                g.DrawLine(&auraPen, s.x1, s.y1, s.x2, s.y2);
            }
            // Core line
            {
                Gdiplus::Pen pen(Gdiplus::Color(a, r, gb, b),
                    std::max(1.0f, s.width));
                pen.SetStartCap(Gdiplus::LineCapRound);
                pen.SetEndCap(Gdiplus::LineCapRound);
                g.DrawLine(&pen, s.x1, s.y1, s.x2, s.y2);
            }
        }

        // Head highlights
        for (const auto& h : frame.heads) {
            const int headA = ClampByte(static_cast<int>(h.alpha * 255.0f));
            const float r = std::max(1.3f, style.strokeWidth * 0.95f);
            Gdiplus::SolidBrush coreBrush(Gdiplus::Color(
                ClampByte(static_cast<int>(headA * 0.82f)), 255, 255, 255));
            g.FillEllipse(&coreBrush, h.x - r, h.y - r, r * 2, r * 2);

            const float r2 = r * 1.85f;
            Gdiplus::SolidBrush glowBrush(Gdiplus::Color(
                ClampByte(static_cast<int>(headA * 0.18f)),
                ClampByte(static_cast<int>(h.strokeR * 255.0f)),
                ClampByte(static_cast<int>(h.strokeG * 255.0f)),
                ClampByte(static_cast<int>(h.strokeB * 255.0f))));
            g.FillEllipse(&glowBrush, h.x - r2, h.y - r2, r2 * 2, r2 * 2);
        }
    }

private:
    RenderParams params_{};
};

REGISTER_RENDERER("helix", HelixRenderer)

} // namespace mousefx
