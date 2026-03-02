#pragma once

#include <cstddef>
#include <cstdint>

namespace mousefx::platform::macos {

struct MacosWasmOverlayPolicy final {
    size_t maxInFlightOverlays = 64;
    uint32_t minImageIntervalMs = 4;
    uint32_t minTextIntervalMs = 8;
};

const MacosWasmOverlayPolicy& GetMacosWasmOverlayPolicy();

} // namespace mousefx::platform::macos
