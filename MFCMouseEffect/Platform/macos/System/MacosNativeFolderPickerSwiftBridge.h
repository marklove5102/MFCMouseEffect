#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t mfx_macos_pick_folder_v1(
    const char* titleUtf8,
    const char* initialPathUtf8,
    char* outPathUtf8,
    int32_t outPathCapacity,
    char* outErrorUtf8,
    int32_t outErrorCapacity);

#ifdef __cplusplus
}
#endif
