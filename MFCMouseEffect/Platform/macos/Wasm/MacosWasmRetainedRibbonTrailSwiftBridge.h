#pragma once

#include <cstdint>

extern "C" {

void* mfx_macos_wasm_retained_ribbon_trail_create_v1(void);

void mfx_macos_wasm_retained_ribbon_trail_release_v1(void* handle);

void mfx_macos_wasm_retained_ribbon_trail_upsert_v1(
    void* handle,
    int32_t frameLeftPx,
    int32_t frameTopPx,
    int32_t squareSizePx,
    const void* nodesPtr,
    uint32_t nodeCount,
    float alpha,
    float glowWidthPx,
    uint32_t fillArgb,
    uint32_t glowArgb,
    uint32_t ttlMs,
    uint32_t blendMode,
    int32_t sortKey,
    uint32_t groupId,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx);
void mfx_macos_wasm_retained_ribbon_trail_set_group_presentation_v1(
    void* handle,
    float alphaMultiplier,
    uint32_t visible);
void mfx_macos_wasm_retained_ribbon_trail_set_effective_clip_rect_v1(
    void* handle,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx);
void mfx_macos_wasm_retained_ribbon_trail_set_effective_clip_rect_v2(
    void* handle,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx,
    uint32_t maskShapeKind,
    float cornerRadiusPx);
void mfx_macos_wasm_retained_ribbon_trail_set_effective_layer_v1(
    void* handle,
    uint32_t blendMode,
    int32_t sortKey);
void mfx_macos_wasm_retained_ribbon_trail_set_effective_translation_v1(
    void* handle,
    float offsetXPx,
    float offsetYPx);

int32_t mfx_macos_wasm_retained_ribbon_trail_is_active_v1(void* handle);

} // extern "C"
