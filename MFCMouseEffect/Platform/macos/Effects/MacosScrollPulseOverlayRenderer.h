#pragma once

#include "MouseFx/Core/Effects/ScrollEffectCompute.h"

#include <string>

namespace mousefx::macos_scroll_pulse {

void CloseAllScrollPulseWindows();
void ShowScrollPulseOverlay(const ScrollEffectRenderCommand& command, const std::string& themeName);

} // namespace mousefx::macos_scroll_pulse
