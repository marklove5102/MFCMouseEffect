#pragma once

#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>

namespace mousefx::macos_scroll_pulse {

void CloseAllScrollPulseWindows();
void ShowScrollPulseOverlay(
    const ScreenPoint& overlayPt,
    bool horizontal,
    int delta,
    const std::string& effectType,
    const std::string& themeName);

} // namespace mousefx::macos_scroll_pulse
