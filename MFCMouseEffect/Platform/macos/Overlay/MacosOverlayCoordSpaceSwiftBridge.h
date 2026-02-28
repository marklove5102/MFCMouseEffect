#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t mfx_macos_overlay_quartz_to_cocoa_v1(
    int32_t inX,
    int32_t inY,
    int32_t* outX,
    int32_t* outY);

#ifdef __cplusplus
}
#endif

