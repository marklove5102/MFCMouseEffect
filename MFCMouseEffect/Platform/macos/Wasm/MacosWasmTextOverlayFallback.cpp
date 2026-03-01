#include "pch.h"

#include "Platform/macos/Wasm/MacosWasmTextOverlayFallback.h"

namespace mousefx::platform::macos::wasm_text_overlay {

MacosTextEffectFallback& SharedFallback() {
    static MacosTextEffectFallback fallback;
    return fallback;
}

} // namespace mousefx::platform::macos::wasm_text_overlay
