#pragma once

#include "Platform/macos/Wasm/MacosWasmTransientOverlay.h"

namespace mousefx::platform::macos {

WasmOverlayRenderResult RenderWasmSpriteBatchOverlay(const WasmSpriteBatchOverlayRequest& request);

} // namespace mousefx::platform::macos
