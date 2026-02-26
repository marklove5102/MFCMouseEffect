#pragma once

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

namespace mousefx::platform::macos {

WasmOverlayRenderResult RenderWasmImageOverlayCore(const WasmImageOverlayRequest& request);

} // namespace mousefx::platform::macos
