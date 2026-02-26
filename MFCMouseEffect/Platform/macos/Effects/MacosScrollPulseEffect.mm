#include "pch.h"

#include "Platform/macos/Effects/MacosScrollPulseEffect.h"

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"
#include "Platform/macos/Effects/MacosScrollPulseOverlayRenderer.h"

#include <utility>

namespace mousefx {

MacosScrollPulseEffect::MacosScrollPulseEffect(std::string effectType, std::string themeName)
    : effectType_(std::move(effectType)),
      themeName_(std::move(themeName)) {
    if (effectType_.empty()) {
        effectType_ = "arrow";
    }
}

MacosScrollPulseEffect::~MacosScrollPulseEffect() {
    Shutdown();
}

bool MacosScrollPulseEffect::Initialize() {
    initialized_ = true;
    return true;
}

void MacosScrollPulseEffect::Shutdown() {
    initialized_ = false;
    macos_scroll_pulse::CloseAllScrollPulseWindows();
}

void MacosScrollPulseEffect::OnScroll(const ScrollEvent& event) {
    if (!initialized_ || event.delta == 0) {
        return;
    }

    const ScreenPoint pt = ScreenToOverlayPoint(event.pt);
    macos_scroll_pulse::ShowScrollPulseOverlay(pt, event.horizontal, event.delta, effectType_, themeName_);
}

} // namespace mousefx
