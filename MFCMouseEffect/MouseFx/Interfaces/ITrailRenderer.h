#pragma once
#include "MouseFx/Core/Protocol/InputTypes.h"
#include <gdiplus.h>
#include <deque>
#include "MouseFx/Core/Config/EffectConfig.h" // For Theme/Color
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace mousefx {

struct TrailPoint {
    ScreenPoint pt{};
    uint64_t addedTime = 0;
    uint32_t fillArgb = 0;
    uint32_t strokeArgb = 0;
    double lineWidthPx = 0.0;
    double intensity = 0.0;
    int durationMs = 0;
};

namespace trail_point_style {

inline int ResolveDurationMs(const TrailPoint& point, int fallbackMs) {
    if (point.durationMs > 0) {
        return std::clamp(point.durationMs, 80, 2000);
    }
    return std::clamp(fallbackMs, 80, 2000);
}

inline float ResolveLineWidthPx(const TrailPoint& point, float fallbackPx) {
    const float width = static_cast<float>(point.lineWidthPx > 0.0 ? point.lineWidthPx : fallbackPx);
    return std::clamp(width, 1.0f, 24.0f);
}

inline Gdiplus::Color ResolveStrokeColor(const TrailPoint& point, const Gdiplus::Color& fallback, int alpha) {
    if (point.strokeArgb == 0) {
        return Gdiplus::Color(alpha, fallback.GetR(), fallback.GetG(), fallback.GetB());
    }
    const BYTE r = static_cast<BYTE>((point.strokeArgb >> 16) & 0xFFu);
    const BYTE g = static_cast<BYTE>((point.strokeArgb >> 8) & 0xFFu);
    const BYTE b = static_cast<BYTE>(point.strokeArgb & 0xFFu);
    return Gdiplus::Color(alpha, r, g, b);
}

inline float ResolveIntensity(const TrailPoint& point, float fallback) {
    const float value = static_cast<float>(point.intensity);
    if (std::isfinite(value)) {
        return std::clamp(value, 0.0f, 1.0f);
    }
    return std::clamp(fallback, 0.0f, 1.0f);
}

} // namespace trail_point_style

class ITrailRenderer {
public:
    virtual ~ITrailRenderer() = default;
    virtual void Render(Gdiplus::Graphics& g, const std::deque<TrailPoint>& points, int width, int height, Gdiplus::Color color, bool isChromatic) = 0;
};

} // namespace mousefx
