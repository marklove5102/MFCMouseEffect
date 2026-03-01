#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseEffect.h"
#include "Platform/macos/Effects/MacosEffectComputeProfileAdapter.h"
#include "Platform/macos/Effects/MacosClickPulseOverlayRenderer.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "MouseFx/Styles/ThemeStyle.h"
#include "MouseFx/Utils/StringUtils.h"
#include "Settings/EmojiUtils.h"

#include <algorithm>
#include <random>
#include <utility>

namespace mousefx {
namespace {

int RandomRange(int minValue, int maxValue) {
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(minValue, maxValue);
    return distribution(generator);
}

std::wstring ResolveDefaultLabel(MouseButton button) {
    switch (button) {
    case MouseButton::Right:
        return L"RIGHT";
    case MouseButton::Middle:
        return L"MIDDLE";
    case MouseButton::Left:
    default:
        return L"LEFT";
    }
}

std::wstring ResolveConfiguredLabel(const TextConfig& config, MouseButton button) {
    if (!config.texts.empty()) {
        const int maxIndex = static_cast<int>(config.texts.size()) - 1;
        return config.texts[RandomRange(0, std::max(maxIndex, 0))];
    }
    return ResolveDefaultLabel(button);
}

uint32_t ResolveConfiguredColor(const TextConfig& config, bool isChromatic, uint32_t fallbackArgb) {
    if (isChromatic) {
        return MakeRandomColor().value;
    }
    if (!config.colors.empty()) {
        const int maxIndex = static_cast<int>(config.colors.size()) - 1;
        return config.colors[RandomRange(0, std::max(maxIndex, 0))].value;
    }
    return fallbackArgb;
}

} // namespace

MacosClickPulseEffect::MacosClickPulseEffect(
    std::string effectType,
    std::string themeName,
    macos_effect_profile::ClickRenderProfile renderProfile,
    TextConfig textConfig)
    : effectType_(std::move(effectType)),
      themeName_(std::move(themeName)),
      renderProfile_(renderProfile),
      textConfig_(std::move(textConfig)) {
    effectType_ = NormalizeClickEffectType(effectType_);
    isChromatic_ = (ToLowerAscii(themeName_) == "chromatic");
}

MacosClickPulseEffect::~MacosClickPulseEffect() {
    Shutdown();
}

bool MacosClickPulseEffect::Initialize() {
    initialized_ = true;
    return true;
}

void MacosClickPulseEffect::Shutdown() {
    initialized_ = false;
    macos_click_pulse::CloseAllClickPulseWindows();
}

void MacosClickPulseEffect::OnClick(const ClickEvent& event) {
    if (!initialized_) {
        return;
    }
    ClickEffectRenderCommand command = ComputeClickEffectRenderCommand(
        ScreenToOverlayPoint(event.pt),
        event.button,
        effectType_,
        macos_effect_compute_profile::BuildClickProfile(renderProfile_));
    if (command.normalizedType == "text") {
        command = BuildTextClickCommand(event, std::move(command));
    }
    macos_click_pulse::ShowClickPulseOverlay(command, themeName_);
}

ClickEffectRenderCommand MacosClickPulseEffect::BuildTextClickCommand(
    const ClickEvent& event,
    ClickEffectRenderCommand command) const {
    const std::wstring resolvedLabel = ResolveConfiguredLabel(textConfig_, event.button);
    const std::string labelUtf8 = Utf16ToUtf8(resolvedLabel.c_str());
    if (!labelUtf8.empty()) {
        command.textLabel = labelUtf8;
    }
    command.textFontFamilyUtf8 = Utf16ToUtf8(textConfig_.fontFamily.c_str());
    command.textEmoji = settings::HasEmojiStarter(resolvedLabel);

    const uint32_t resolvedTextArgb = ResolveConfiguredColor(textConfig_, isChromatic_, command.strokeArgb);
    command.fillArgb = 0x00000000u;
    command.strokeArgb = resolvedTextArgb;
    command.glowArgb = resolvedTextArgb;
    command.textFontSizePx = std::clamp(
        static_cast<double>(textConfig_.fontSize) * (96.0 / 72.0),
        8.0,
        220.0);
    command.textFloatDistancePx = std::clamp(
        static_cast<double>(textConfig_.floatDistance),
        0.0,
        360.0);
    command.animationDurationSec = std::clamp(
        static_cast<double>(textConfig_.durationMs) / 1000.0,
        0.16,
        3.0);
    command.baseOpacity = 1.0;
    return command;
}

} // namespace mousefx
