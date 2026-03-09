#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

void* mfx_macos_wasm_path_stroke_overlay_create_v1(
    double frameX,
    double frameY,
    double frameSize,
    const void* nodesPtr,
    uint32_t nodeCount,
    double lineWidthPx,
    unsigned int strokeArgb,
    unsigned int glowArgb,
    double alphaScale,
    double durationSec,
    uint8_t lineJoin,
    uint8_t lineCap,
    uint32_t blendMode,
    int32_t sortKey,
    uint32_t groupId,
    double clipLeftPx,
    double clipTopPx,
    double clipWidthPx,
    double clipHeightPx);

void mfx_macos_wasm_path_stroke_overlay_show_v1(void* windowHandle);

#ifdef __cplusplus
}
#endif
