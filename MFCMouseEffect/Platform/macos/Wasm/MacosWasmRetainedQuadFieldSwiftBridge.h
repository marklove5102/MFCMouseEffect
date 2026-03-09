#pragma once

#include <cstdint>

extern "C" {

void* mfx_macos_wasm_retained_quad_field_create_v1(void);
void mfx_macos_wasm_retained_quad_field_release_v1(void* handle);
void mfx_macos_wasm_retained_quad_field_upsert_v1(
    void* handle,
    int32_t frameLeftPx,
    int32_t frameTopPx,
    int32_t squareSizePx,
    const void* spriteBytes,
    uint32_t spriteCount,
    const char* const* imagePathUtf8,
    uint32_t ttlMs,
    uint32_t blendMode,
    int32_t sortKey,
    uint32_t groupId,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx);
void mfx_macos_wasm_retained_quad_field_set_group_presentation_v1(
    void* handle,
    float alphaMultiplier,
    uint32_t visible);
void mfx_macos_wasm_retained_quad_field_set_effective_clip_rect_v1(
    void* handle,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx);
void mfx_macos_wasm_retained_quad_field_set_effective_clip_rect_v2(
    void* handle,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx,
    uint32_t maskShapeKind,
    float cornerRadiusPx);
void mfx_macos_wasm_retained_quad_field_set_effective_layer_v1(
    void* handle,
    uint32_t blendMode,
    int32_t sortKey);
void mfx_macos_wasm_retained_quad_field_set_effective_translation_v1(
    void* handle,
    float offsetXPx,
    float offsetYPx);
uint32_t mfx_macos_wasm_retained_quad_field_is_active_v1(void* handle);

}
