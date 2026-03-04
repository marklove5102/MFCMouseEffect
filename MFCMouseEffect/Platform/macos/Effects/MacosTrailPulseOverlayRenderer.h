#pragma once

#include "MouseFx/Core/Effects/TrailEffectCompute.h"

#include <string>

namespace mousefx::macos_trail_pulse {

void CloseAllTrailPulseWindows();
void ShowTrailPulseOverlay(const TrailEffectRenderCommand& command, const std::string& themeName);

} // namespace mousefx::macos_trail_pulse
