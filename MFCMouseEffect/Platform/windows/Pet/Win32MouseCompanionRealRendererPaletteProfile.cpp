#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererPaletteProfile.h"

namespace mousefx::windows {
namespace {

Gdiplus::Color MakeColor(BYTE a, BYTE r, BYTE g, BYTE b) {
    return Gdiplus::Color(a, r, g, b);
}

Gdiplus::Color PickBodyFill(const std::string& skinVariantId) {
    if (skinVariantId == "cream") {
        return MakeColor(255, 255, 243, 212);
    }
    if (skinVariantId == "night") {
        return MakeColor(255, 73, 87, 126);
    }
    if (skinVariantId == "strawberry") {
        return MakeColor(255, 255, 228, 235);
    }
    return MakeColor(255, 239, 244, 255);
}

} // namespace

Win32MouseCompanionRealRendererPaletteProfile BuildWin32MouseCompanionRealRendererPaletteProfile(
    const Win32MouseCompanionRealRendererSceneRuntime& runtime) {
    Win32MouseCompanionRealRendererPaletteProfile profile{};
    profile.glowColor = MakeColor(255, 82, 170, 255);
    profile.baseBodyFill = PickBodyFill(runtime.assets->appearanceProfileSkinVariantId);
    profile.bodyStroke = MakeColor(255, 70, 98, 152);
    profile.headFill = MakeColor(255, 255, 250, 246);
    profile.headFillRear = MakeColor(255, 241, 234, 232);
    profile.earFill = MakeColor(255, 255, 247, 235);
    profile.earFillRear = MakeColor(255, 235, 226, 214);
    profile.earInner = MakeColor(255, 255, 201, 214);
    profile.eyeFill = MakeColor(255, 38, 44, 62);
    profile.mouthFill = MakeColor(255, 106, 84, 114);
    profile.blushRgb = MakeColor(255, 255, 171, 194);
    profile.accentFill = MakeColor(255, 111, 219, 255);
    profile.pedestalFill = MakeColor(105, 59, 77, 115);
    profile.badgeReadyFill = MakeColor(255, 111, 229, 178);
    profile.badgePendingFill = MakeColor(255, 255, 189, 97);
    profile.accessoryFill = MakeColor(255, 255, 221, 97);
    profile.accessoryStroke = MakeColor(255, 144, 98, 38);
    return profile;
}

} // namespace mousefx::windows
