#pragma once

namespace mousefx::macos_click_pulse {

void RegisterClickPulseWindow(void* windowHandle);
bool TakeClickPulseWindow(void* windowHandle);
void CloseAllClickPulseWindowsNow();

} // namespace mousefx::macos_click_pulse
