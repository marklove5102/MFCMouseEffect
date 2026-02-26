#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseEffect.h"
#include "Platform/macos/Effects/MacosClickPulseOverlayRenderer.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"

#include <utility>

namespace mousefx {

MacosClickPulseEffect::MacosClickPulseEffect(std::string effectType, std::string themeName)
    : effectType_(std::move(effectType)),
      themeName_(std::move(themeName)) {
    if (effectType_.empty()) {
        effectType_ = "ripple";
    }
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
    const ScreenPoint overlayPt = ScreenToOverlayPoint(event.pt);
    macos_click_pulse::ShowClickPulseOverlay(overlayPt, event.button, effectType_, themeName_);
}

} // namespace mousefx
