#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

void* mfx_macos_wasm_polyline_overlay_create_v1(
    double frameX,
    double frameY,
    double frameSize,
    const float* pointsXY,
    uint32_t pointCount,
    double lineWidthPx,
    unsigned int strokeArgb,
    unsigned int glowArgb,
    double alphaScale,
    double durationSec,
    int closed);

void mfx_macos_wasm_polyline_overlay_show_v1(void* windowHandle);

#ifdef __cplusplus
}
#endif
