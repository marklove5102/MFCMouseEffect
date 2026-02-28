#include "pch.h"

#include "Platform/macos/Overlay/MacosOverlayCoordSpaceConversion.h"
#include "Platform/macos/Overlay/MacosOverlayCoordSpaceSwiftBridge.h"

namespace mousefx::macos_overlay_coord_conversion {

bool TryConvertQuartzToCocoa(const ScreenPoint& input, ScreenPoint* output) {
#if !defined(__APPLE__)
    (void)input;
    (void)output;
    return false;
#else
    if (!output) {
        return false;
    }
    int32_t outX = input.x;
    int32_t outY = input.y;
    if (mfx_macos_overlay_quartz_to_cocoa_v1(input.x, input.y, &outX, &outY) == 0) {
        return false;
    }
    output->x = outX;
    output->y = outY;
    return true;
#endif
}

} // namespace mousefx::macos_overlay_coord_conversion
