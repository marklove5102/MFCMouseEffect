#pragma once

#include "Platform/macos/Wasm/MacosWasmOverlayRuntime.h"

#include <vector>

namespace mousefx::platform::macos {

WasmOverlayAdmissionResult TryAcquireWasmOverlaySlotState(WasmOverlayKind kind);
void ReleaseWasmOverlaySlotState();
size_t GetWasmOverlayInFlightCountState();
WasmOverlayThrottleCounters GetWasmOverlayThrottleCountersState();
void RegisterWasmOverlayWindowState(void* windowHandle);
bool TakeWasmOverlayWindowState(void* windowHandle);
std::vector<void*> ResetAndTakeAllWasmOverlayWindowsState();

} // namespace mousefx::platform::macos
