#pragma once

#include <cstddef>
#include <cstdint>

namespace mousefx::platform::macos {

struct MacosWasmOverlayPolicy final {
    size_t maxInFlightOverlays = 384;
    uint32_t minImageIntervalMs = 0;
    uint32_t minTextIntervalMs = 4;
};

const MacosWasmOverlayPolicy& GetMacosWasmOverlayPolicy();

} // namespace mousefx::platform::macos
