#pragma once

#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>

namespace mousefx::macos_click_pulse {

void CloseAllClickPulseWindows();
void ShowClickPulseOverlay(const ScreenPoint& overlayPt, MouseButton button, const std::string& themeName);

} // namespace mousefx::macos_click_pulse
