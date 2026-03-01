#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* mfx_macos_wasm_text_overlay_create_v1(
    double x,
    double y,
    double width,
    double height,
    double fontSize,
    unsigned int argb,
    const char* textUtf8);
void mfx_macos_wasm_text_overlay_show_v1(void* overlayHandle);
void mfx_macos_wasm_text_overlay_release_v1(void* overlayHandle);

#ifdef __cplusplus
}
#endif
