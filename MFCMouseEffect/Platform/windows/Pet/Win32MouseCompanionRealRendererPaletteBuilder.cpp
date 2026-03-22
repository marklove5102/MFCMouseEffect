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

BYTE BlendChannel(BYTE from, BYTE to, float mix) {
    const float clampedMix = std::clamp(mix, 0.0f, 1.0f);
    const float blended =
        static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * clampedMix;
    return static_cast<BYTE>(std::clamp(std::lround(blended), 0L, 255L));
}

Gdiplus::Color BlendToward(const Gdiplus::Color& base, const Gdiplus::Color& tint, float mix) {
    return MakeColor(
        BlendChannel(base.GetA(), tint.GetA(), mix),
        BlendChannel(base.GetR(), tint.GetR(), mix),
        BlendChannel(base.GetG(), tint.GetG(), mix),
        BlendChannel(base.GetB(), tint.GetB(), mix));
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
    const float shadowAlphaBias = runtime.follow ? style.followShadowAlphaBias
        : runtime.hold                           ? style.holdShadowAlphaBias
        : runtime.scroll                         ? style.scrollShadowAlphaBias
        : runtime.drag                           ? style.dragShadowAlphaBias
                                                 : 0.0f;
    const float pedestalAlphaBias = runtime.follow ? style.followPedestalAlphaBias
        : runtime.hold                             ? style.holdPedestalAlphaBias
        : runtime.scroll                           ? style.scrollPedestalAlphaBias
        : runtime.drag                             ? style.dragPedestalAlphaBias
                                                   : 0.0f;
    const auto actionTint = profile.overlayAccentColor;
    scene.glowColor = BlendToward(palette.glowColor, actionTint, emphasis * style.glowActionTintMix);
    scene.bodyFill = BlendToward(baseBody, actionTint, emphasis * style.bodyActionTintMix);
    scene.bodyFillRear = Darken(scene.bodyFill, style.bodyRearDarkenFactor);
    scene.bodyStroke = MakeColor(
        static_cast<BYTE>(std::clamp(style.bodyStrokeBaseAlpha + emphasis * style.bodyStrokeActionAlphaScale, 0.0f, 255.0f)),
        BlendChannel(palette.bodyStroke.GetR(), actionTint.GetR(), emphasis * style.strokeActionTintMix),
        BlendChannel(palette.bodyStroke.GetG(), actionTint.GetG(), emphasis * style.strokeActionTintMix),
        BlendChannel(palette.bodyStroke.GetB(), actionTint.GetB(), emphasis * style.strokeActionTintMix));
    scene.headFill = BlendToward(palette.headFill, actionTint, emphasis * style.headActionTintMix);
    scene.headFillRear = BlendToward(palette.headFillRear, actionTint, emphasis * style.headActionTintMix * 0.7f);
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
    scene.tailFill = Darken(scene.bodyFill, style.tailDarkenFactor);
    scene.accentFill = MakeColor(
        static_cast<BYTE>(std::clamp(style.accentBaseAlpha + emphasis * style.accentActionAlphaScale, 0.0f, 255.0f)),
        BlendChannel(palette.accentFill.GetR(), actionTint.GetR(), emphasis * style.accentActionTintMix),
        BlendChannel(palette.accentFill.GetG(), actionTint.GetG(), emphasis * style.accentActionTintMix),
        BlendChannel(palette.accentFill.GetB(), actionTint.GetB(), emphasis * style.accentActionTintMix));
    scene.shadowFill = MakeColor(
        static_cast<BYTE>(std::clamp(
            style.shadowBaseAlpha + shadowEmphasis * style.shadowActionAlphaScale + shadowAlphaBias,
            0.0f,
            255.0f)),
        BlendChannel(palette.pedestalFill.GetR(), actionTint.GetR(), emphasis * style.shadowActionTintMix),
        BlendChannel(palette.pedestalFill.GetG(), actionTint.GetG(), emphasis * style.shadowActionTintMix),
        BlendChannel(palette.pedestalFill.GetB(), actionTint.GetB(), emphasis * style.shadowActionTintMix));
    scene.pedestalFill = MakeColor(
        static_cast<BYTE>(std::clamp(
            style.pedestalBaseAlpha + shadowEmphasis * style.pedestalActionAlphaScale + pedestalAlphaBias,
            0.0f,
            255.0f)),
        BlendChannel(palette.pedestalFill.GetR(), actionTint.GetR(), emphasis * style.pedestalActionTintMix),
        BlendChannel(palette.pedestalFill.GetG(), actionTint.GetG(), emphasis * style.pedestalActionTintMix),
        BlendChannel(palette.pedestalFill.GetB(), actionTint.GetB(), emphasis * style.pedestalActionTintMix));
    scene.badgeReadyFill = palette.badgeReadyFill;
    scene.badgePendingFill = palette.badgePendingFill;
    scene.accessoryFill = palette.accessoryFill;
    scene.accessoryStroke = palette.accessoryStroke;
}

} // namespace mousefx::windows
