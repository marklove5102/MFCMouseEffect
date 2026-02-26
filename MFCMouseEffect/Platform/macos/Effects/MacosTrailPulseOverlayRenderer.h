#pragma once

#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>

namespace mousefx::macos_trail_pulse {

void CloseAllTrailPulseWindows();
void ShowTrailPulseOverlay(
    const ScreenPoint& overlayPt,
    double deltaX,
    double deltaY,
    const std::string& effectType,
    const std::string& themeName);

} // namespace mousefx::macos_trail_pulse
