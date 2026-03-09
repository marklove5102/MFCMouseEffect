#pragma once

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

namespace mousefx::platform::macos {

WasmOverlayRenderResult RenderWasmPathStrokeOverlay(const WasmPathStrokeOverlayRequest& request);

} // namespace mousefx::platform::macos
