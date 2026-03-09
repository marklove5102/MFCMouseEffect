#pragma once

#include <cstdint>

extern "C" {

void* mfx_macos_wasm_glow_batch_overlay_create_v1(
    double frameX,
    double frameY,
    double frameSize,
    const void* particlesPtr,
    uint32_t particleCount,
    double durationSec,
    uint32_t blendMode,
    int32_t sortKey,
    uint32_t groupId);

void mfx_macos_wasm_glow_batch_overlay_show_v1(void* windowHandle);

} // extern "C"
