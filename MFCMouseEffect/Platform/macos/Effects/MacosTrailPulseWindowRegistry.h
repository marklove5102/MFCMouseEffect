#pragma once

#include <cstddef>

namespace mousefx::macos_trail_pulse {

void RegisterTrailPulseWindow(void* windowHandle);
bool TakeTrailPulseWindow(void* windowHandle);
void CloseAllTrailPulseWindowsNow();

} // namespace mousefx::macos_trail_pulse
