#pragma once

#include <array>

#include <gdiplus.h>

#include "Platform/windows/Pet/Win32MouseCompanionVisualState.h"

namespace mousefx::windows {

struct Win32MouseCompanionPlaceholderScene {
    float centerX{0.0f};
    float bodyLeanPx{0.0f};
    Gdiplus::Color bodyFill{};
    Gdiplus::Color bodyStroke{};
    Gdiplus::Color headFill{};
    Gdiplus::Color earInner{};
    Gdiplus::Color accent{};
    float glowAlpha{0.0f};
    Gdiplus::RectF bodyRect{};
    Gdiplus::RectF headRect{};
    std::array<Gdiplus::PointF, 4> leftEar{};
    std::array<Gdiplus::PointF, 4> rightEar{};
    Gdiplus::RectF leftHandRect{};
    Gdiplus::RectF rightHandRect{};
    Gdiplus::RectF leftLegRect{};
    Gdiplus::RectF rightLegRect{};
    bool modelAssetAvailable{false};
    bool actionLibraryAvailable{false};
    bool poseBadgeVisible{false};
    bool accessoryVisible{false};
    Gdiplus::Color eyeColor{};
    Gdiplus::Color mouthColor{};
    Gdiplus::Color blushColor{};
    float eyeHeight{0.0f};
};

Win32MouseCompanionPlaceholderScene BuildWin32MouseCompanionPlaceholderScene(
    const Win32MouseCompanionVisualState& state,
    int width,
    int height);

} // namespace mousefx::windows
