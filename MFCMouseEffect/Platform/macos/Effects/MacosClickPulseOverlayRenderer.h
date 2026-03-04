#pragma once

#include "MouseFx/Core/Effects/ClickEffectCompute.h"

#include <string>

namespace mousefx::macos_click_pulse {

void CloseAllClickPulseWindows();
void ShowClickPulseOverlay(const ClickEffectRenderCommand& command, const std::string& themeName);

} // namespace mousefx::macos_click_pulse
