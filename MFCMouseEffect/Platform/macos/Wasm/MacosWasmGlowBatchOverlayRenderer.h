#pragma once

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

namespace mousefx::platform::macos {

WasmOverlayRenderResult RenderWasmGlowBatchOverlay(const WasmGlowBatchOverlayRequest& request);

} // namespace mousefx::platform::macos
