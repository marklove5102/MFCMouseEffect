#pragma once

#include <cstdint>

extern "C" {

void* mfx_macos_wasm_retained_sprite_emitter_create_v1(void);

void mfx_macos_wasm_retained_sprite_emitter_release_v1(void* handle);

void mfx_macos_wasm_retained_sprite_emitter_upsert_v1(
    void* handle,
    int32_t frameLeftPx,
    int32_t frameTopPx,
    int32_t squareSizePx,
    float localX,
    float localY,
    const char* imagePathUtf8,
    float emissionRatePerSec,
    float directionRad,
    float spreadRad,
    float speedMin,
    float speedMax,
    float sizeMinPx,
    float sizeMaxPx,
    float alphaMin,
    float alphaMax,
    uint32_t tintArgb,
    uint32_t applyTint,
    float rotationMinRad,
    float rotationMaxRad,
    float accelerationX,
    float accelerationY,
    uint32_t emitterTtlMs,
    uint32_t particleLifeMs,
    uint16_t maxParticles,
    uint32_t blendMode,
    int32_t sortKey,
    uint32_t groupId,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx);
void mfx_macos_wasm_retained_sprite_emitter_set_group_presentation_v1(
    void* handle,
    float alphaMultiplier,
    uint32_t visible);
void mfx_macos_wasm_retained_sprite_emitter_set_effective_clip_rect_v1(
    void* handle,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx);
void mfx_macos_wasm_retained_sprite_emitter_set_effective_clip_rect_v2(
    void* handle,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx,
    uint32_t maskShapeKind,
    float cornerRadiusPx);
void mfx_macos_wasm_retained_sprite_emitter_set_effective_layer_v1(
    void* handle,
    uint32_t blendMode,
    int32_t sortKey);
void mfx_macos_wasm_retained_sprite_emitter_set_effective_translation_v1(
    void* handle,
    float offsetXPx,
    float offsetYPx);

int32_t mfx_macos_wasm_retained_sprite_emitter_is_active_v1(void* handle);

} // extern "C"
