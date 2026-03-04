#pragma once

#include "MouseFx/Interfaces/ITrailRenderer.h"
#include "MouseFx/Core/Effects/TrailStyleCompute.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Utils/TrailColor.h"
#include "MouseFx/Utils/TrailMath.h"
#include "MouseFx/Utils/TimeUtils.h"
#include <algorithm>
#include <cmath>

namespace mousefx {

class StreamerTrailRenderer final : public ITrailRenderer {
public:
    StreamerTrailRenderer(int durationMs, const TrailRendererParamsConfig& params)
        : durationMs_(durationMs), params_(params.streamer), idle_(params.idleFade) {}

    void Render(Gdiplus::Graphics& g,
                const std::deque<TrailPoint>& points,
                int /*width*/,
                int /*height*/,
                Gdiplus::Color color,
                bool isChromatic) override {
        if (points.size() < 2) return;

        const uint64_t now = NowMs();
        int fadeStart = idle_.startMs > 0 ? idle_.startMs : 50;
        int fadeEnd = idle_.endMs > 0 ? idle_.endMs : 260;
        if (fadeEnd <= fadeStart) fadeEnd = fadeStart + 1;
        const float idleFactor = trail_math::IdleFadeFactor(now, points.back().addedTime, fadeStart, fadeEnd);

        const size_t n = points.size();

        // Neon streamer: two-pass stroke (outer glow + inner core) with head-weighted thickness.
        for (size_t i = 0; i + 1 < n; ++i) {
            const auto& p1 = points[i];
            const auto& p2 = points[i + 1];
            const ScreenPoint lp1 = ScreenToOverlayPoint(p1.pt);
            const ScreenPoint lp2 = ScreenToOverlayPoint(p2.pt);

            const float t = (n <= 2) ? 1.0f : (float)i / (float)(n - 1); // 0 tail -> 1 head

            const int pointDurationMs = trail_point_style::ResolveDurationMs(p1, durationMs_);
            const uint64_t age = now - p1.addedTime;
            float life = 1.0f - (static_cast<float>(age) / static_cast<float>(pointDurationMs));
            life = trail_math::Clamp01(life) * idleFactor;
            if (life <= 0.0f) continue;

            const float baseWidth = trail_point_style::ResolveLineWidthPx(p1, 4.0f);
            const float widthScale = std::max(0.35f, baseWidth / 4.0f);
            const auto metrics = trail_style_compute::ComputeStreamerSegmentMetrics(
                t,
                life,
                static_cast<double>(params_.headPower));
            const float w = static_cast<float>(metrics.widthPx * widthScale);

            int alphaCore = static_cast<int>(std::lround(255.0 * metrics.coreOpacity));
            int alphaGlow = static_cast<int>(std::lround(255.0 * metrics.glowOpacity));
            alphaCore = (int)trail_math::Clamp((float)alphaCore, 0.0f, 255.0f);
            alphaGlow = (int)trail_math::Clamp((float)alphaGlow, 0.0f, 255.0f);

            Gdiplus::Color cCore = trail_point_style::ResolveStrokeColor(p1, color, alphaCore);
            Gdiplus::Color cGlow = trail_point_style::ResolveStrokeColor(p1, color, alphaGlow);

            if (isChromatic) {
                float hue = static_cast<float>(trail_style_compute::ComputeTrailChromaticHueDeg(
                    now,
                    1,
                    static_cast<uint32_t>(i),
                    0));
                cCore = trail_color::HslToRgbColor(hue, 0.95f, 0.62f, (BYTE)alphaCore);
                cGlow = trail_color::HslToRgbColor(hue, 0.95f, 0.58f, (BYTE)alphaGlow);
            }

            // Outer glow
            {
                Gdiplus::Pen glowPen(cGlow, w * params_.glowWidthScale);
                glowPen.SetStartCap(Gdiplus::LineCapRound);
                glowPen.SetEndCap(Gdiplus::LineCapRound);
                glowPen.SetLineJoin(Gdiplus::LineJoinRound);
                g.DrawLine(&glowPen,
                           lp1.x,
                           lp1.y,
                           lp2.x,
                           lp2.y);
            }

            // Inner core
            {
                Gdiplus::Pen corePen(cCore, std::max(1.5f, w * params_.coreWidthScale));
                corePen.SetStartCap(Gdiplus::LineCapRound);
                corePen.SetEndCap(Gdiplus::LineCapRound);
                corePen.SetLineJoin(Gdiplus::LineJoinRound);
                g.DrawLine(&corePen,
                           lp1.x,
                           lp1.y,
                           lp2.x,
                           lp2.y);
            }
        }
    }

private:
    int durationMs_ = 420;
    StreamerTrailParams params_{};
    IdleFadeParams idle_{};
};

} // namespace mousefx
