#pragma once

#include "MouseFx/Core/Overlay/OverlayCoordSpace.h"

#include <cstddef>
#include <cstdint>

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace mousefx::platform::macos {

#if defined(__APPLE__)
struct WasmTextOverlayLayout {
    uint32_t durationMs = 0;
    CGFloat fontSize = 0;
    CGFloat width = 0;
    CGFloat height = 0;
    NSRect frame = NSZeroRect;
};

WasmTextOverlayLayout BuildWasmTextOverlayLayout(
    const ScreenPoint& screenPt,
    size_t utf8Length,
    float scale,
    uint32_t lifeMs);

void ConfigureWasmTextOverlayPanel(NSPanel* panel, CGFloat height);
NSTextField* CreateWasmTextOverlayLabel(
    CGFloat width,
    CGFloat height,
    CGFloat fontSize,
    uint32_t argb,
    NSString* value);
#endif

} // namespace mousefx::platform::macos
