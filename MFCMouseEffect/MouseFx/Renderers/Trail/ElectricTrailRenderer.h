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

class ElectricTrailRenderer final : public ITrailRenderer {
public:
    ElectricTrailRenderer(int durationMs, const TrailRendererParamsConfig& params)
        : durationMs_(durationMs), params_(params.electric), idle_(params.idleFade) {}

    void Render(Gdiplus::Graphics& g,
                const std::deque<TrailPoint>& points,
                int /*width*/,
                int /*height*/,
                Gdiplus::Color color,
                bool isChromatic) override {
        if (points.size() < 2) return;

        const uint64_t now = NowMs();
        const uint64_t frameKey = now / 30; // stabilize jitter within ~1-2 frames
        int fadeStart = idle_.startMs > 0 ? idle_.startMs : 40;
        int fadeEnd = idle_.endMs > 0 ? idle_.endMs : 180;
        if (fadeEnd <= fadeStart) fadeEnd = fadeStart + 1;
        const float idleFactor = trail_math::IdleFadeFactor(now, points.back().addedTime, fadeStart, fadeEnd);

        for (size_t i = 0; i + 1 < points.size(); ++i) {
            const auto& p1 = points[i];
            const auto& p2 = points[i + 1];
            const ScreenPoint lp1 = ScreenToOverlayPoint(p1.pt);
            const ScreenPoint lp2 = ScreenToOverlayPoint(p2.pt);

            const int pointDurationMs = trail_point_style::ResolveDurationMs(p1, durationMs_);
            const uint64_t age = now - p1.addedTime;
            float life = 1.0f - (static_cast<float>(age) / static_cast<float>(pointDurationMs));
            life = trail_math::Clamp01(life) * idleFactor;
            if (life <= 0.0f) continue;

            float x1 = (float)lp1.x;
            float y1 = (float)lp1.y;
            float x2 = (float)lp2.x;
            float y2 = (float)lp2.y;

            float dx = x2 - x1;
            float dy = y2 - y1;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.5f) continue;

            const auto metrics = trail_style_compute::ComputeElectricSegmentMetrics(
                frameKey,
                static_cast<uint32_t>(i),
                life,
                len,
                static_cast<double>(params_.amplitudeScale),
                static_cast<double>(params_.forkChance));

            float invLen = 1.0f / len;
            float nx = -dy * invLen;
            float ny = dx * invLen;

            float o1 = static_cast<float>(metrics.jitterA);
            float o2 = static_cast<float>(metrics.jitterB);

            Gdiplus::PointF a(x1, y1);
            Gdiplus::PointF b(x1 + dx * 0.35f + nx * o1, y1 + dy * 0.35f + ny * o1);
            Gdiplus::PointF c(x1 + dx * 0.70f + nx * o2, y1 + dy * 0.70f + ny * o2);
            Gdiplus::PointF d(x2, y2);

            const float baseWidth = trail_point_style::ResolveLineWidthPx(
                p1,
                static_cast<float>(metrics.coreWidthPx > 0.0 ? metrics.coreWidthPx : 2.2));
            const float coreW = std::max(1.0f, baseWidth * life);
            const float glowW = std::max(
                coreW,
                static_cast<float>((metrics.glowWidthPx > 0.0 ? metrics.glowWidthPx : metrics.coreWidthPx * 3.0) * std::max(0.35f, baseWidth / 2.2f)));

            int alpha = static_cast<int>(std::lround(255.0 * metrics.coreOpacity));
            alpha = (int)trail_math::Clamp((float)alpha, 0.0f, 255.0f);
            const int glowAlpha = static_cast<int>(std::lround(255.0 * metrics.glowOpacity));

            Gdiplus::Color cGlow = trail_point_style::ResolveStrokeColor(p1, color, glowAlpha);
            Gdiplus::Color cCore(alpha, 255, 255, 255);

            if (isChromatic) {
                float hue = static_cast<float>(trail_style_compute::ComputeTrailChromaticHueDeg(
                    now,
                    2,
                    static_cast<uint32_t>(i),
                    0));
                cGlow = trail_color::HslToRgbColor(hue, 1.0f, 0.60f, (BYTE)glowAlpha);
                cCore = trail_color::HslToRgbColor(std::fmod(hue + 30.0f, 360.0f), 1.0f, 0.75f, (BYTE)alpha);
            } else {
                // Let theme tint the glow slightly, but keep a white core for "electric" identity.
                cGlow = Gdiplus::Color(glowAlpha,
                                       (BYTE)std::max<int>(70, cGlow.GetR()),
                                       (BYTE)std::max<int>(200, cGlow.GetG()),
                                       (BYTE)std::max<int>(220, cGlow.GetB()));
            }

            // Glow stroke
            {
                Gdiplus::Pen penGlow(cGlow, glowW);
                penGlow.SetLineJoin(Gdiplus::LineJoinRound);
                penGlow.SetStartCap(Gdiplus::LineCapRound);
                penGlow.SetEndCap(Gdiplus::LineCapRound);
                g.DrawLine(&penGlow, a, b);
                g.DrawLine(&penGlow, b, c);
                g.DrawLine(&penGlow, c, d);
            }

            // Core stroke
            {
                Gdiplus::Pen penCore(cCore, coreW);
                penCore.SetLineJoin(Gdiplus::LineJoinRound);
                penCore.SetStartCap(Gdiplus::LineCapRound);
                penCore.SetEndCap(Gdiplus::LineCapRound);
                g.DrawLine(&penCore, a, b);
                g.DrawLine(&penCore, b, c);
                g.DrawLine(&penCore, c, d);
            }

            // Occasional fork to make it feel less like a "jittery ribbon".
            if (metrics.emitFork) {
                float t = static_cast<float>(metrics.forkT);
                Gdiplus::PointF base(x1 + dx * t, y1 + dy * t);
                float forkLen = static_cast<float>(metrics.forkLengthPx);
                float forkSide = static_cast<float>(metrics.forkSide < 0 ? -1 : 1);
                Gdiplus::PointF tip(base.X + nx * forkSide * forkLen + dx * 0.05f, base.Y + ny * forkSide * forkLen + dy * 0.05f);

                const float widthScale = std::max(0.35f, baseWidth / 2.2f);
                const float forkWidth = std::max(1.0f, static_cast<float>(metrics.forkWidthPx * widthScale));
                const int forkAlpha = static_cast<int>(std::lround(255.0 * metrics.forkOpacity));
                Gdiplus::Color forkColor = cGlow;
                forkColor.SetValue((static_cast<uint32_t>(std::clamp(forkAlpha, 0, 255)) << 24) | (forkColor.GetValue() & 0x00FFFFFFu));
                Gdiplus::Pen forkPen(forkColor, forkWidth);
                forkPen.SetLineJoin(Gdiplus::LineJoinRound);
                forkPen.SetStartCap(Gdiplus::LineCapRound);
                forkPen.SetEndCap(Gdiplus::LineCapRound);
                g.DrawLine(&forkPen, base, tip);
            }
        }
    }

private:
    int durationMs_ = 280;
    ElectricTrailParams params_{};
    IdleFadeParams idle_{};
};

} // namespace mousefx
