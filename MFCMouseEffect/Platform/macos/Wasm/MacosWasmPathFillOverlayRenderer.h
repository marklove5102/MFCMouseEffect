#pragma once

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

namespace mousefx::platform::macos {

WasmOverlayRenderResult RenderWasmPathFillOverlay(const WasmPathFillOverlayRequest& request);

} // namespace mousefx::platform::macos
