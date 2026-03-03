#include "pch.h"

#include "MouseFx/Core/Effects/ScrollEffectCompute.h"
#include "MouseFx/Styles/ThemeStyle.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Platform/macos/Effects/MacosScrollPulseEffect.h"

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "Platform/macos/Effects/MacosScrollPulseOverlayRenderer.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace mousefx {
namespace {

ScrollEffectProfile BuildScrollProfileFromThemeStyle(const RippleStyle& style) {
    ScrollEffectProfile profile{};
    const int windowSize = std::clamp(style.windowSize, 64, 640);
    profile.verticalSizePx = windowSize;
    profile.horizontalSizePx = windowSize;
    profile.geometryReferenceSizePx = windowSize;
    profile.baseStartRadiusPx = std::clamp(static_cast<double>(style.startRadius), 0.0, 640.0);
    profile.baseEndRadiusPx = std::clamp(static_cast<double>(style.endRadius), 1.0, 800.0);
    profile.baseStrokeWidthPx = std::clamp(static_cast<double>(style.strokeWidth), 0.1, 64.0);
    profile.baseDurationSec = std::clamp(static_cast<double>(style.durationMs) / 1000.0, 0.05, 5.0);
    profile.perStrengthStepSec = 0.0;
    profile.closePaddingMs = 0;
    profile.baseOpacity = 1.0;
    profile.defaultDurationScale = 1.0;
    profile.helixDurationScale = 1.0;
    profile.twinkleDurationScale = 1.0;
    profile.defaultSizeScale = 1.0;
    profile.helixSizeScale = 1.0;
    profile.twinkleSizeScale = 1.0;
    const ScrollEffectDirectionColorProfile color{style.fill.value, style.stroke.value};
    profile.horizontalPositive = color;
    profile.horizontalNegative = color;
    profile.verticalPositive = color;
    profile.verticalNegative = color;
    return profile;
}

} // namespace

MacosScrollPulseEffect::MacosScrollPulseEffect(
    std::string effectType,
    std::string themeName)
    : effectType_(std::move(effectType)),
      themeName_(std::move(themeName)) {
    effectType_ = NormalizeScrollEffectType(effectType_);
    style_ = GetThemePalette(themeName_).scroll;
    computeProfile_ = BuildScrollProfileFromThemeStyle(style_);
    isChromatic_ = (ToLowerAscii(themeName_) == "chromatic");
}

MacosScrollPulseEffect::~MacosScrollPulseEffect() {
    Shutdown();
}

bool MacosScrollPulseEffect::Initialize() {
    initialized_ = true;
    lastEmitTickMs_ = 0;
    pendingDelta_ = 0;
    return true;
}

void MacosScrollPulseEffect::Shutdown() {
    initialized_ = false;
    lastEmitTickMs_ = 0;
    pendingDelta_ = 0;
    macos_scroll_pulse::CloseAllScrollPulseWindows();
}

void MacosScrollPulseEffect::OnScroll(const ScrollEvent& event) {
    if (!initialized_ || event.delta == 0) {
        return;
    }

    const ScrollEffectInputShaperProfile shaper = ResolveScrollInputShaperProfile(effectType_);
    pendingDelta_ += event.delta;
    const uint64_t now = CurrentTickMs();
    if (lastEmitTickMs_ != 0 && (now - lastEmitTickMs_) < shaper.emitIntervalMs) {
        return;
    }
    lastEmitTickMs_ = now;
    int effectiveDelta = event.delta;
    if (pendingDelta_ != 0) {
        effectiveDelta = pendingDelta_;
        pendingDelta_ = 0;
    }

    const RippleStyle runtimeStyle = isChromatic_ ? MakeRandomStyle(style_) : style_;
    const ScrollEffectProfile runtimeProfile = isChromatic_
        ? BuildScrollProfileFromThemeStyle(runtimeStyle)
        : computeProfile_;
    const ScrollEffectRenderCommand command = ComputeScrollEffectRenderCommand(
        ScreenToOverlayPoint(event.pt),
        event.horizontal,
        effectiveDelta,
        effectType_,
        runtimeProfile);
    macos_scroll_pulse::ShowScrollPulseOverlay(command, themeName_);
}

uint64_t MacosScrollPulseEffect::CurrentTickMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace mousefx
