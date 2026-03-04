#include "pch.h"

#include "MouseFx/Core/Effects/HoverEffectCompute.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Styles/ThemeStyle.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/macos/Effects/MacosHoverPulseEffect.h"
#include "Platform/macos/Effects/MacosHoverPulseOverlayRenderer.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mousefx {
namespace {

int ResolveScaledWindowSize(int baseWindowSize, int sizeScalePercent) {
    const double sizeScale = std::clamp(static_cast<double>(sizeScalePercent) / 100.0, 0.5, 2.0);
    return std::clamp(static_cast<int>(std::lround(static_cast<double>(baseWindowSize) * sizeScale)), 64, 720);
}

HoverEffectProfile BuildHoverProfileFromThemeStyle(const RippleStyle& style, int sizeScalePercent) {
    HoverEffectProfile profile{};
    // Keep hover default tighter than raw style window to match Windows visual weight.
    const int rawWindowSize = std::clamp(style.windowSize, 64, 640);
    const int tunedBaseWindowSize = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(rawWindowSize) * 0.72)),
        56,
        560);
    const int windowSize = ResolveScaledWindowSize(tunedBaseWindowSize, sizeScalePercent);
    profile.sizePx = windowSize;
    const double durationSec = std::clamp(static_cast<double>(style.durationMs) / 1000.0, 0.05, 8.0);
    profile.breatheDurationSec = durationSec;
    profile.spinDurationSec = durationSec;
    profile.baseOpacity = 1.0;
    profile.glowSizeScale = 1.0;
    profile.tubesSizeScale = 1.0;
    profile.glowBreatheScale = 1.0;
    profile.tubesBreatheScale = 1.0;
    profile.tubesSpinScale = 1.0;
    profile.colors.glowFillArgb = style.fill.value;
    profile.colors.glowStrokeArgb = style.stroke.value;
    profile.colors.tubesStrokeArgb = style.stroke.value;
    return profile;
}

} // namespace

MacosHoverPulseEffect::MacosHoverPulseEffect(
    std::string effectType,
    std::string themeName,
    int sizeScalePercent)
    : effectType_(std::move(effectType)),
      themeName_(std::move(themeName)),
      sizeScalePercent_(std::clamp(sizeScalePercent, 50, 200)) {
    effectType_ = NormalizeHoverEffectType(effectType_);
    style_ = GetThemePalette(themeName_).hover;
    computeProfile_ = BuildHoverProfileFromThemeStyle(style_, sizeScalePercent_);
    isChromatic_ = (ToLowerAscii(themeName_) == "chromatic");
}

MacosHoverPulseEffect::~MacosHoverPulseEffect() {
    Shutdown();
}

bool MacosHoverPulseEffect::Initialize() {
    initialized_ = true;
    return true;
}

void MacosHoverPulseEffect::Shutdown() {
    initialized_ = false;
    macos_hover_pulse::CloseHoverPulseOverlay();
}

void MacosHoverPulseEffect::OnHoverStart(const ScreenPoint& pt) {
    if (!initialized_) {
        return;
    }
    const RippleStyle runtimeStyle = isChromatic_ ? MakeRandomStyle(style_) : style_;
    const HoverEffectProfile runtimeProfile = isChromatic_
        ? BuildHoverProfileFromThemeStyle(runtimeStyle, sizeScalePercent_)
        : computeProfile_;
    const HoverEffectRenderCommand command =
        ComputeHoverEffectRenderCommand(
            ScreenToOverlayPoint(pt),
            effectType_,
            runtimeProfile);
    macos_hover_pulse::ShowHoverPulseOverlay(command, themeName_);
}

void MacosHoverPulseEffect::OnHoverEnd() {
    macos_hover_pulse::CloseHoverPulseOverlay();
}

} // namespace mousefx
