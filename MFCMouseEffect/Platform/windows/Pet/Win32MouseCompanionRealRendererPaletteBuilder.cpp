#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererPaletteBuilder.h"

#include <algorithm>
#include <cmath>

namespace mousefx::windows {
namespace {

Gdiplus::Color MakeColor(BYTE a, BYTE r, BYTE g, BYTE b) {
    return Gdiplus::Color(a, r, g, b);
}

Gdiplus::Color Darken(const Gdiplus::Color& color, float factor) {
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    const auto scale = [clamped](BYTE c) -> BYTE {
        return static_cast<BYTE>(std::lround(static_cast<float>(c) * (1.0f - clamped)));
    };
    return MakeColor(color.GetA(), scale(color.GetR()), scale(color.GetG()), scale(color.GetB()));
}

} // namespace

void BuildWin32MouseCompanionRealRendererPalette(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime,
    const Win32MouseCompanionRealRendererMotionProfile& profile,
    const Win32MouseCompanionRealRendererStyleProfile& style,
    const Win32MouseCompanionRealRendererPaletteProfile& palette,
    Win32MouseCompanionRealRendererScene& scene) {
    const auto baseBody = palette.baseBodyFill;
    scene.glowColor = palette.glowColor;
    scene.bodyFill = baseBody;
    scene.bodyFillRear = Darken(baseBody, style.bodyRearDarkenFactor);
    scene.bodyStroke = palette.bodyStroke;
    scene.headFill = palette.headFill;
    scene.headFillRear = palette.headFillRear;
    scene.earFill = palette.earFill;
    scene.earFillRear = palette.earFillRear;
    scene.earInner = palette.earInner;
    scene.eyeFill = palette.eyeFill;
    scene.mouthFill = palette.mouthFill;
    scene.blushFill = MakeColor(
        static_cast<BYTE>(std::clamp(profile.blushAlpha, 0.0f, 255.0f)),
        palette.blushRgb.GetR(),
        palette.blushRgb.GetG(),
        palette.blushRgb.GetB());
    scene.tailFill = Darken(baseBody, style.tailDarkenFactor);
    scene.accentFill = palette.accentFill;
    scene.pedestalFill = palette.pedestalFill;
    scene.badgeReadyFill = palette.badgeReadyFill;
    scene.badgePendingFill = palette.badgePendingFill;
    scene.accessoryFill = palette.accessoryFill;
    scene.accessoryStroke = palette.accessoryStroke;
}

} // namespace mousefx::windows
