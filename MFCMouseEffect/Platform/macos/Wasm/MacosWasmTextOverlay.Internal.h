#pragma once

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"

#include <cstddef>
#include <cstdint>

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace mousefx::platform::macos {

#if defined(__APPLE__)
struct WasmTextOverlayLayout {
    uint32_t durationMs = 0;
    CGFloat fontSize = 0;
    CGFloat width = 0;
    CGFloat height = 0;
    CGRect frame = CGRectZero;
};

WasmTextOverlayLayout BuildWasmTextOverlayLayout(
    const ScreenPoint& screenPt,
    size_t utf8Length,
    float scale,
    uint32_t lifeMs);
#endif

} // namespace mousefx::platform::macos
