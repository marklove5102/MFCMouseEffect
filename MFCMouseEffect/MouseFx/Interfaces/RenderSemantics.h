#pragma once

#include <cstdint>

namespace mousefx {

enum class RenderBlendMode : uint8_t {
    Normal = 0,
    Screen = 1,
    Add = 2,
};

struct RenderClipRect final {
    float leftPx = 0.0f;
    float topPx = 0.0f;
    float widthPx = 0.0f;
    float heightPx = 0.0f;
};

struct RenderSemantics final {
    RenderBlendMode blendMode = RenderBlendMode::Normal;
    int32_t sortKey = 0;
    uint32_t groupId = 0u;
    RenderClipRect clipRect{};
};

inline bool UsesScreenLikeBlend(RenderBlendMode blendMode) {
    return blendMode == RenderBlendMode::Screen || blendMode == RenderBlendMode::Add;
}

inline bool HasClipRect(const RenderSemantics& semantics) {
    return semantics.clipRect.widthPx > 0.0f && semantics.clipRect.heightPx > 0.0f;
}

} // namespace mousefx
