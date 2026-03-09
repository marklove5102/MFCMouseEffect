#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

void* mfx_macos_wasm_sprite_batch_overlay_create_v1(
    double frameX,
    double frameY,
    double frameSize,
    const void* spriteBytes,
    uint32_t spriteCount,
    const char* const* imagePathUtf8Ptrs,
    double durationSec,
    uint32_t blendMode,
    int32_t sortKey,
    uint32_t groupId,
    double clipLeftPx,
    double clipTopPx,
    double clipWidthPx,
    double clipHeightPx);

void mfx_macos_wasm_sprite_batch_overlay_show_v1(void* windowHandle);

#ifdef __cplusplus
}
#endif
