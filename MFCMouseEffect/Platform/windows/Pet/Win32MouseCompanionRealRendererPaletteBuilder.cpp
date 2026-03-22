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
    const float emphasis = std::max({profile.actionIntensity, profile.reactiveIntensity, profile.scrollIntensity});
    const float shadowEmphasis = std::clamp(
        emphasis + (runtime.hold ? 0.18f : 0.0f) + (runtime.follow ? 0.08f : 0.0f),
        0.0f,
        1.0f);
    scene.glowColor = palette.glowColor;
    scene.bodyFill = baseBody;
    scene.bodyFillRear = Darken(baseBody, style.bodyRearDarkenFactor);
    scene.bodyStroke = MakeColor(
        static_cast<BYTE>(std::clamp(style.bodyStrokeBaseAlpha + emphasis * style.bodyStrokeActionAlphaScale, 0.0f, 255.0f)),
        palette.bodyStroke.GetR(),
        palette.bodyStroke.GetG(),
        palette.bodyStroke.GetB());
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
    scene.accentFill = MakeColor(
        static_cast<BYTE>(std::clamp(style.accentBaseAlpha + emphasis * style.accentActionAlphaScale, 0.0f, 255.0f)),
        palette.accentFill.GetR(),
        palette.accentFill.GetG(),
        palette.accentFill.GetB());
    scene.shadowFill = MakeColor(
        static_cast<BYTE>(std::clamp(style.shadowBaseAlpha + shadowEmphasis * style.shadowActionAlphaScale, 0.0f, 255.0f)),
        palette.pedestalFill.GetR(),
        palette.pedestalFill.GetG(),
        palette.pedestalFill.GetB());
    scene.pedestalFill = MakeColor(
        static_cast<BYTE>(std::clamp(style.pedestalBaseAlpha + shadowEmphasis * style.pedestalActionAlphaScale, 0.0f, 255.0f)),
        palette.pedestalFill.GetR(),
        palette.pedestalFill.GetG(),
        palette.pedestalFill.GetB());
    scene.badgeReadyFill = palette.badgeReadyFill;
    scene.badgePendingFill = palette.badgePendingFill;
    scene.accessoryFill = palette.accessoryFill;
    scene.accessoryStroke = palette.accessoryStroke;
}

} // namespace mousefx::windows
