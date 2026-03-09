#pragma once

#include <cstdint>

extern "C" {

void* mfx_macos_wasm_retained_particle_emitter_create_v1(void);

void mfx_macos_wasm_retained_particle_emitter_release_v1(void* handle);

void mfx_macos_wasm_retained_particle_emitter_upsert_v1(
    void* handle,
    int32_t frameLeftPx,
    int32_t frameTopPx,
    int32_t squareSizePx,
    float localX,
    float localY,
    float emissionRatePerSec,
    float directionRad,
    float spreadRad,
    float speedMin,
    float speedMax,
    float radiusMinPx,
    float radiusMaxPx,
    float alphaMin,
    float alphaMax,
    uint32_t colorArgb,
    float accelerationX,
    float accelerationY,
    uint32_t emitterTtlMs,
    uint32_t particleLifeMs,
    uint16_t maxParticles,
    uint8_t particleStyle,
    uint8_t emissionMode,
    uint8_t spawnShape,
    float spawnRadiusX,
    float spawnRadiusY,
    float spawnInnerRatio,
    float dragPerSecond,
    float turbulenceAccel,
    float turbulenceFrequencyHz,
    float turbulencePhaseJitter,
    float sizeStartScale,
    float sizeEndScale,
    float alphaStartScale,
    float alphaEndScale,
    uint32_t colorStartArgb,
    uint32_t colorEndArgb,
    uint32_t blendMode,
    int32_t sortKey,
    uint32_t groupId,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx);
void mfx_macos_wasm_retained_particle_emitter_set_group_presentation_v1(
    void* handle,
    float alphaMultiplier,
    uint32_t visible);
void mfx_macos_wasm_retained_particle_emitter_set_effective_clip_rect_v1(
    void* handle,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx);
void mfx_macos_wasm_retained_particle_emitter_set_effective_clip_rect_v2(
    void* handle,
    float clipLeftPx,
    float clipTopPx,
    float clipWidthPx,
    float clipHeightPx,
    uint32_t maskShapeKind,
    float cornerRadiusPx);
void mfx_macos_wasm_retained_particle_emitter_set_effective_layer_v1(
    void* handle,
    uint32_t blendMode,
    int32_t sortKey);
void mfx_macos_wasm_retained_particle_emitter_set_effective_translation_v1(
    void* handle,
    float offsetXPx,
    float offsetYPx);

int32_t mfx_macos_wasm_retained_particle_emitter_is_active_v1(void* handle);

} // extern "C"
