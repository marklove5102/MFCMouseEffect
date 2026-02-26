#pragma once

#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>

namespace mousefx::macos_hover_pulse {

void ShowHoverPulseOverlay(
    const ScreenPoint& overlayPt,
    const std::string& effectType,
    const std::string& themeName);
void CloseHoverPulseOverlay();

} // namespace mousefx::macos_hover_pulse
