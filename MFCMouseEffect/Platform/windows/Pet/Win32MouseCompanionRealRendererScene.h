#pragma once

#include <array>

#include <gdiplus.h>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererScene final {
    float centerX{0.0f};
    float centerY{0.0f};
    float bodyTiltDeg{0.0f};
    float facingSign{1.0f};
    float glowAlpha{0.0f};
    Gdiplus::Color glowColor{};
    Gdiplus::Color bodyFill{};
    Gdiplus::Color bodyFillRear{};
    Gdiplus::Color bodyStroke{};
    Gdiplus::Color headFill{};
    Gdiplus::Color headFillRear{};
    Gdiplus::Color earFill{};
    Gdiplus::Color earFillRear{};
    Gdiplus::Color earInner{};
    Gdiplus::Color eyeFill{};
    Gdiplus::Color mouthFill{};
    Gdiplus::Color blushFill{};
    Gdiplus::Color tailFill{};
    Gdiplus::Color accentFill{};
    Gdiplus::Color pedestalFill{};
    Gdiplus::Color badgeReadyFill{};
    Gdiplus::Color badgePendingFill{};
    Gdiplus::Color accessoryFill{};
    Gdiplus::Color accessoryStroke{};
    Gdiplus::RectF glowRect{};
    Gdiplus::RectF shadowRect{};
    Gdiplus::RectF bodyRect{};
    Gdiplus::RectF chestRect{};
    Gdiplus::RectF headRect{};
    Gdiplus::RectF tailRect{};
    std::array<Gdiplus::PointF, 4> leftEar{};
    std::array<Gdiplus::PointF, 4> rightEar{};
    Gdiplus::RectF leftHandRect{};
    Gdiplus::RectF rightHandRect{};
    Gdiplus::RectF leftLegRect{};
    Gdiplus::RectF rightLegRect{};
    Gdiplus::RectF leftEyeRect{};
    Gdiplus::RectF rightEyeRect{};
    Gdiplus::RectF mouthRect{};
    Gdiplus::RectF noseRect{};
    Gdiplus::RectF leftBlushRect{};
    Gdiplus::RectF rightBlushRect{};
    Gdiplus::RectF pedestalRect{};
    std::array<Gdiplus::RectF, 3> laneBadgeRects{};
    std::array<bool, 3> laneReady{};
    bool poseBadgeVisible{false};
    Gdiplus::RectF poseBadgeRect{};
    bool accessoryVisible{false};
    std::array<Gdiplus::PointF, 5> accessoryStar{};
};

} // namespace mousefx::windows
