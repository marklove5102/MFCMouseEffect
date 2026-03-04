#pragma once

#include "MouseFx/Core/Effects/HoverEffectCompute.h"

#include <cstddef>
#include <string>

namespace mousefx::macos_hover_pulse {

void ShowHoverPulseOverlay(const HoverEffectRenderCommand& command, const std::string& themeName);
void CloseHoverPulseOverlay();
size_t GetActiveHoverPulseWindowCount();

} // namespace mousefx::macos_hover_pulse
