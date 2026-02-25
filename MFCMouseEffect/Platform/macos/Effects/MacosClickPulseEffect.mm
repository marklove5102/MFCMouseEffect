#include "pch.h"

#include "Platform/macos/Effects/MacosClickPulseEffect.h"
#include "Platform/macos/Effects/MacosClickPulseOverlayRenderer.h"
#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"

namespace mousefx {

MacosClickPulseEffect::MacosClickPulseEffect(std::string themeName)
    : themeName_(std::move(themeName)) {
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
    macos_click_pulse::ShowClickPulseOverlay(overlayPt, event.button, themeName_);
}

} // namespace mousefx
