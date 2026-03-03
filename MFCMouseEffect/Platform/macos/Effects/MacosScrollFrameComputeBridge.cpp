/// MacosScrollFrameComputeBridge.cpp
/// C ABI bridge: exposes shared Core helix/twinkle frame computation to Swift.
///
/// Design: instead of returning C structs with pointers (which may have
/// ABI issues with Swift @_silgen_name), we use an opaque-handle pattern
/// where Swift calls compute, then pulls data via getter functions.

#if defined(__APPLE__)

#include "MouseFx/Core/Effects/ScrollHelixFrameCompute.h"
#include "MouseFx/Core/Effects/ScrollTwinkleFrameCompute.h"
#include <cstdint>

using namespace mousefx;

extern "C" {

// ── Helix ─────────────────────────────────────────────────────

// Persistent frame state per helix instance.
struct MfxHelixComputeState {
    ScrollHelixFrameData frame;
};

void* mfx_scroll_helix_state_create() {
    return new MfxHelixComputeState();
}

void mfx_scroll_helix_state_destroy(void* state) {
    delete static_cast<MfxHelixComputeState*>(state);
}

void mfx_scroll_helix_compute(
    void* state,
    float t, uint64_t elapsedMs, int32_t sizePx,
    float directionRad, float intensity,
    float startRadius, float endRadius, float strokeWidth,
    float strokeR, float strokeG, float strokeB,
    float glowR, float glowG, float glowB
) {
    if (!state) return;
    auto* s = static_cast<MfxHelixComputeState*>(state);
    s->frame = ComputeHelixFrame(
        t, elapsedMs, sizePx, directionRad, intensity,
        startRadius, endRadius, strokeWidth,
        strokeR, strokeG, strokeB, glowR, glowG, glowB);
}

int32_t mfx_scroll_helix_segment_count(void* state) {
    if (!state) return 0;
    return static_cast<int32_t>(static_cast<MfxHelixComputeState*>(state)->frame.segments.size());
}

// Copy segment i into flat output parameters.
void mfx_scroll_helix_get_segment(
    void* state, int32_t i,
    float* x1, float* y1, float* x2, float* y2,
    float* width, float* r, float* g, float* b, float* a
) {
    auto* s = static_cast<MfxHelixComputeState*>(state);
    if (!s || i < 0 || i >= (int32_t)s->frame.segments.size()) return;
    const auto& seg = s->frame.segments[static_cast<size_t>(i)];
    *x1 = seg.x1; *y1 = seg.y1; *x2 = seg.x2; *y2 = seg.y2;
    *width = seg.width;
    *r = seg.r; *g = seg.g; *b = seg.b; *a = seg.a;
}

int32_t mfx_scroll_helix_head_count(void* state) {
    if (!state) return 0;
    return static_cast<int32_t>(static_cast<MfxHelixComputeState*>(state)->frame.heads.size());
}

void mfx_scroll_helix_get_head(
    void* state, int32_t i,
    float* x, float* y, float* alpha,
    float* strokeR, float* strokeG, float* strokeB
) {
    auto* s = static_cast<MfxHelixComputeState*>(state);
    if (!s || i < 0 || i >= (int32_t)s->frame.heads.size()) return;
    const auto& h = s->frame.heads[static_cast<size_t>(i)];
    *x = h.x; *y = h.y; *alpha = h.alpha;
    *strokeR = h.strokeR; *strokeG = h.strokeG; *strokeB = h.strokeB;
}

// ── Twinkle ───────────────────────────────────────────────────

struct MfxTwinkleComputeState {
    ScrollTwinkleState engine;
    ScrollTwinkleFrameData frame;
};

void* mfx_scroll_twinkle_state_create() {
    return new MfxTwinkleComputeState();
}

void mfx_scroll_twinkle_state_destroy(void* state) {
    delete static_cast<MfxTwinkleComputeState*>(state);
}

void mfx_scroll_twinkle_state_start(
    void* state, float cx, float cy, float dirRad, float intensity
) {
    if (!state) return;
    static_cast<MfxTwinkleComputeState*>(state)->engine.Start(cx, cy, dirRad, intensity);
}

void mfx_scroll_twinkle_compute(
    void* state,
    float t, uint64_t elapsedMs, int32_t sizePx,
    float strokeR, float strokeG, float strokeB,
    float glowR, float glowG, float glowB
) {
    if (!state) return;
    auto* s = static_cast<MfxTwinkleComputeState*>(state);
    s->frame = s->engine.ComputeFrame(
        t, elapsedMs, sizePx,
        strokeR, strokeG, strokeB, glowR, glowG, glowB);
}

int32_t mfx_scroll_twinkle_particle_count(void* state) {
    if (!state) return 0;
    return static_cast<int32_t>(static_cast<MfxTwinkleComputeState*>(state)->frame.particles.size());
}

void mfx_scroll_twinkle_get_particle(
    void* state, int32_t i,
    float* x, float* y, float* prevX, float* prevY, float* drawSize,
    float* coreR, float* coreG, float* coreB, float* coreAlpha,
    float* trailR, float* trailG, float* trailB, float* trailAlpha,
    float* glowAlpha
) {
    auto* s = static_cast<MfxTwinkleComputeState*>(state);
    if (!s || i < 0 || i >= (int32_t)s->frame.particles.size()) return;
    const auto& p = s->frame.particles[static_cast<size_t>(i)];
    *x = p.x; *y = p.y; *prevX = p.prevX; *prevY = p.prevY; *drawSize = p.drawSize;
    *coreR = p.coreR; *coreG = p.coreG; *coreB = p.coreB; *coreAlpha = p.coreAlpha;
    *trailR = p.trailR; *trailG = p.trailG; *trailB = p.trailB; *trailAlpha = p.trailAlpha;
    *glowAlpha = p.glowAlpha;
}

} // extern "C"

#endif // __APPLE__
